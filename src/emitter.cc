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
#include <assert.h>
#include <math.h>
#include <unistd.h>

#include "./fluent/emitter.hpp"
#include "./debug.h"

namespace fluent {
  const int Emitter::WAIT_MAX = 120 * 1000;

  Emitter::Emitter(const std::string &host, int port) :
    retry_limit_(0)
  {
    // Setup socket.
    std::stringstream ss;
    ss << port;
    this->sock_ = new Socket(host, ss.str());

    ::pthread_create(&(this->th_), NULL, Emitter::run_thread, this);    
  }

  Emitter::~Emitter() {
  }

  bool Emitter::emit(Message *msg) {
    bool rc = this->queue_.push(msg);
    return rc;
  }

  void Emitter::set_queue_limit(size_t limit) {
    this->queue_.set_limit(limit);
  }
  
  bool Emitter::connect() {
    static const bool DBG = false;

    for (size_t i = 0; this->retry_limit_ == 0 || i < this->retry_limit_;
         i++) {
      if (this->sock_->connect()) {
        debug(DBG, "connected");
        return true;
      }
      int wait_msec_max =
        static_cast<int>(pow(2, static_cast<double>(i)) * 1000);
      if (wait_msec_max > WAIT_MAX) {
        wait_msec_max = WAIT_MAX;
      }
      int wait_msec = random() % wait_msec_max;

      debug(DBG, "reconnect after %d msec...", wait_msec);
      usleep(wait_msec * 1000);
    }

    this->errmsg_ = this->sock_->errmsg();
    delete this->sock_;
    this->sock_ = nullptr;
    return false;
  }

  void* Emitter::run_thread(void *obj) {
    Emitter *emitter = static_cast<Emitter*>(obj);
    emitter->loop();
    return NULL;
  }

  void Emitter::loop() {
    while (true) {

      if (!this->sock_->is_connected()) {
        this->connect(); // TODO: handle failure of retry
      }

      Message *root = this->queue_.pop();
      for(Message *msg = root; msg; msg = root->next()) {
        msgpack::sbuffer buf;
        msgpack::packer <msgpack::sbuffer> pk(&buf);
        msg->to_msgpack(&pk);

        while(!this->sock_->send(buf.data(), buf.size())) {
          // std::cerr << "socket error: " << this->sock_->errmsg() << std::endl;
          this->connect(); // TODO: handle failure of retry
        }

        delete root;
      }
    }
  }

  MsgQueue::MsgQueue() : msg_head_(nullptr), msg_tail_(nullptr),
                         count_(0), limit_(1000) {
    // Setup pthread.
    ::pthread_mutex_init(&(this->mutex_), NULL);
    ::pthread_cond_init(&(this->cond_), NULL);
  }
  MsgQueue::~MsgQueue() {
  }
  bool MsgQueue::push(Message *msg) {
    static const bool DBG = false;
    bool rc = true;

    ::pthread_mutex_lock (&(this->mutex_));
    debug(DBG, "entered lock");

    if (this->count_ < this->limit_) {
      if (this->msg_head_) {
        assert(this->msg_tail_);
        this->msg_tail_->attach(msg);
      } else {
        this->msg_head_ = msg;
      }
      this->msg_tail_ = msg;
      this->count_++;
      debug(DBG, "send signal");
      ::pthread_cond_signal (&(this->cond_));
    } else {
      // queue is full.
      rc = false;
    }
    ::pthread_mutex_unlock (&(this->mutex_));
    debug(DBG, "left lock");
    
    return rc;
  }
  Message* MsgQueue::pop() {
    static const bool DBG = false;
    Message *msg;
    
    debug(DBG, "entering lock");
    ::pthread_mutex_lock(&(this->mutex_));
    debug(DBG, "entered lock");

    if (this->msg_head_ == nullptr) {
      debug(DBG, "entered wait");
      ::pthread_cond_wait(&(this->cond_), &(this->mutex_));
      debug(DBG, "left wait");
    }

    assert(this->count_ > 0);
    msg = this->msg_head_;
    this->msg_head_ = this->msg_tail_ = nullptr;
    this->count_--;
    
    ::pthread_mutex_unlock(&(this->mutex_));
    debug(DBG, "left lock");

    return msg;
  }

  void MsgQueue::set_limit(size_t limit) {
    ::pthread_mutex_lock(&(this->mutex_));
    this->limit_ = limit;
    ::pthread_mutex_unlock(&(this->mutex_));    
  }

}
