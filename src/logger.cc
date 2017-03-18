/*-
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
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "./fluent/logger.hpp"
#include "./fluent/message.hpp"
#include "./fluent/emitter.hpp"
#include "./debug.h"

namespace fluent {
  Logger::Logger() {
#ifdef _WIN32
#ifndef FLUENTSKIPSTARTWINSOCK
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
#endif // FLUENTSKIPSTARTWINSOCK
#endif // _WIN32
  }
  Logger::~Logger() {
    // delete not used messages.
    for (auto it = this->msg_set_.begin(); it != this->msg_set_.end(); it++) {
      delete *it;
    }
    for (size_t i = 0; i < this->emitter_.size(); i++) {
      delete this->emitter_[i];
    }
    std::for_each(this->queue_.begin(), this->queue_.end(),
                  [](MsgQueue* const &x) { delete x; });

#ifdef _WIN32
#ifndef FLUENTSKIPSTARTWINSOCK
    WSACleanup();
#endif // FLUENTSKIPSTARTWINSOCK
#endif // _WIN32
  }

  void Logger::new_forward(const std::string &host, int port) {
    Emitter *e = new InetEmitter(host, port);
    this->emitter_.push_back(e);
  }
  void Logger::new_forward(const std::string &host, const std::string &port) {
    Emitter *e = new InetEmitter(host, port);
    this->emitter_.push_back(e);
  }
  void Logger::new_dumpfile(const std::string &fname) {
    Emitter *e = new FileEmitter(fname);
    this->emitter_.push_back(e);
  }
  void Logger::new_dumpfile(int fd) {
    Emitter *e = new FileEmitter(fd);
    this->emitter_.push_back(e);
  }
  MsgQueue* Logger::new_msgqueue() {
    MsgQueue *q = new MsgQueue();
    this->queue_.push_back(q);
    Emitter *e = new QueueEmitter(q);
    this->emitter_.push_back(e);
    return q;
  }
  
  
  Message* Logger::retain_message(const std::string &tag) {
    Message *msg;
    if (this->tag_prefix_.empty()) {
      msg = new Message(tag);
    } else {
      std::string cattag = this->tag_prefix_ + "." + tag;
      msg = new Message(cattag);
    }

    this->msg_set_.insert(msg);
    return msg;
  }


  bool Logger::emit(Message *msg) {
    if (this->msg_set_.find(msg) == this->msg_set_.end()) {
      this->errmsg_ = "invalid Message instance, "
        "should be got by Logger::retain_message()";
      return false;
    }

    this->msg_set_.erase(msg);

    bool rc = true;
    if (this->emitter_.size() == 1) {
      rc = this->emitter_[0]->emit(msg);
    } else if (this->emitter_.size() > 1) {
      for (size_t i = 0; i < this->emitter_.size() - 1; i++) {
        Message *cloned_msg = msg->clone();
        rc &= this->emitter_[i]->emit(cloned_msg);
      }
      rc &= this->emitter_[this->emitter_.size() - 1]->emit(msg);
    }
    
    return rc;
  }

  void Logger::set_queue_limit(size_t limit) {
    for (size_t i = 0; i < this->emitter_.size(); i++) {
      this->emitter_[i]->set_queue_limit(limit);
    }
  }

  void Logger::set_tag_prefix(const std::string &tag_prefix) {
    this->tag_prefix_ = tag_prefix;
  }
}
