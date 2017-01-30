/*
 * Copyright (c) 2015 Masayoshi Mizutani <mizutani@sfc.wide.ad.jp>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sstream>
#include <iostream>
#include <msgpack.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <random>

#include "./fluent/emitter.hpp"
#include "./debug.h"

namespace fluent {
  // ----------------------------------------------------------------
  // Emitter
  Emitter::Emitter() {
  }

  Emitter::~Emitter() {
  }

  bool Emitter::emit(Message *msg) {
    debug(DBG, "emit %p", msg);
    bool rc = this->queue_.push(msg);
    return rc;
  }

  void Emitter::set_queue_limit(size_t limit) {
    this->queue_.set_limit(limit);
  }

  void* Emitter::run_thread(void *obj) {
    Emitter *emitter = static_cast<Emitter*>(obj);
    emitter->worker();
    return NULL;
  }

  void Emitter::start_worker() {
    ::pthread_create(&(this->th_), NULL, Emitter::run_thread, this);    
  }
  void Emitter::stop_worker() {
    // ::pthread_cancel(this->th_);
    this->queue_.term();
    ::pthread_join(this->th_, nullptr);
  }

  // ----------------------------------------------------------------
  // InetEmitter
  const int InetEmitter::WAIT_MAX = 30 * 1000;

  InetEmitter::InetEmitter(const std::string &host, int port) :
    Emitter(), retry_limit_(0)
  {
    // Setup socket.
    std::stringstream ss;
    ss << port;
    init(host, ss.str());
  }
  InetEmitter::InetEmitter(const std::string &host,
                           const std::string &port) :
    Emitter(), retry_limit_(0)
  {
    init(host, port);
  }
  void InetEmitter::init(const std::string &host,
						 const std::string &port) {
    // Setup random engine
    mt_rand = std::mt19937(random_device());
    rand_dist = std::uniform_int_distribution<int>(0, INT_MAX);

    // Setup socket.
    this->sock_ = new Socket(host, port);
    this->start_worker();

  }
  InetEmitter::~InetEmitter() {
    this->stop_worker();
    delete this->sock_;
  }

  bool InetEmitter::connect() {
    for (size_t i = 0; this->retry_limit_ == 0 || i < this->retry_limit_;
         i++) {

      if (this->queue_.is_term()) {
        // Going to shutdown.
        return false;
      }
      
      if (this->sock_->connect()) {
        debug(DBG, "connected");
        return true;
      }
      int wait_msec_max =
        static_cast<int>(pow(2, static_cast<double>(i)) * 1000);
      if (wait_msec_max > WAIT_MAX) {
        wait_msec_max = WAIT_MAX;
      }
      int wait_msec = rand_dist(mt_rand) % wait_msec_max;

      debug(DBG, "reconnect after %d msec...", wait_msec);
      usleep(wait_msec * 1000);
    }

    this->set_errmsg(this->sock_->errmsg());
    delete this->sock_;
    this->sock_ = nullptr;
    return false;
  }

  void InetEmitter::worker() {
    if (!this->sock_->is_connected()) {
      this->connect(); // TODO: handle failure of retry
    }

    Message *root;
    bool abort_loop = false;
    
    while (nullptr != (root = this->queue_.bulk_pop())) {
      for(Message *msg = root; msg; msg = msg->next()) {
        msgpack::sbuffer buf;
        msgpack::packer <msgpack::sbuffer> pk(&buf);
        msg->to_msgpack(&pk);

        debug(DBG, "sending msg %p", msg);
        while(!this->sock_->send(buf.data(), buf.size())) {
          debug(DBG, "socket error: %s", this->sock_->errmsg().c_str());
          // std::cerr << "socket error: " << this->sock_->errmsg() << std::endl;
          if (!this->connect()) {
            abort_loop = true;
            break;
          }
        }

        if (abort_loop) {
          break;
        }
        
        debug(false, "sent %p", msg);
      }
      delete root;
    }
  }

  // ----------------------------------------------------------------
  // FileEmitter
  FileEmitter::FileEmitter(const std::string &fname) :
    Emitter(), enabled_(false), opened_(false) {
    // Setup socket.

    this->fd_ = ::open(fname.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (this->fd_ < 0) {
      this->set_errmsg(strerror(errno));
    } else {
      this->opened_ = true;
      this->enabled_ = true;
      this->start_worker();
    }
  }
  FileEmitter::FileEmitter(int fd) : Emitter(), fd_(fd),
                                     enabled_(false), opened_(false) {
#ifndef _WIN32
    if (fcntl(fd, F_GETFL) < 0 && errno == EBADF) {
      this->set_errmsg("Invalid file descriptor");
	} else
#endif
	{
      this->enabled_ = true;
      this->start_worker();
    }
  }
    
  FileEmitter::~FileEmitter() {
    this->stop_worker();
    if (this->enabled_ && this->opened_) {
      ::close(this->fd_);
    }
  }

  void FileEmitter::worker() {
    assert(this->enabled_);
    
    Message *root;
    while (nullptr != (root = this->queue_.bulk_pop())) {
      for(Message *msg = root; msg; msg = msg->next()) {
        msgpack::sbuffer buf;
        msgpack::packer <msgpack::sbuffer> pk(&buf);
        msg->to_msgpack(&pk);

        int rc = ::write(this->fd_, buf.data(), buf.size());
        if (rc < 0) {
          this->set_errmsg(strerror(errno));
        }
      }
      delete root;
    }
  }


  // ----------------------------------------------------------------
  // FileEmitter
  QueueEmitter::QueueEmitter(MsgQueue *q) : q_(q) {
  }
  QueueEmitter::~QueueEmitter() {
  }
  void QueueEmitter::worker() {    
    // nothing to do.
  } 
  bool QueueEmitter::emit(Message *msg) {
    return this->q_->push(msg);
  }
  
}
