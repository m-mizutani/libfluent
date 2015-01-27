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
#include "./fluent/socket.hpp"
#include "./fluent/message.hpp"
#include "./debug.h"

namespace fluent {
  const int Logger::WAIT_MAX = 120 * 1000;
  Logger::Logger(const std::string &host, int port) :
    host_(host),
    port_(port),
    sock_(nullptr),
    retry_max_(20)
  {
  }
  Logger::~Logger() {
  }

  Message* Logger::retain_message() {
    Message *msg = new Message();
    this->msg_set_.insert(msg);
    return msg;
  }

  bool Logger::connect() {
    std::stringstream ss;
    ss << this->port_;
    this->sock_ = new Socket(this->host_, ss.str());
    for (size_t i = 0; this->retry_max_ == 0 || i < this->retry_max_; i++) {
      if (this->sock_->connect()) {
        return true;
      }
      int wait_msec_max =
        static_cast<int>(pow(2, static_cast<double>(i)) * 1000);
      if (wait_msec_max > WAIT_MAX) {
        wait_msec_max = WAIT_MAX;
      }      
      int wait_msec = random() % wait_msec_max;
      
      debug(false, "reconnect after %d msec...", wait_msec);
      usleep(wait_msec * 1000);
    }
    
    this->errmsg_ = this->sock_->errmsg();
    delete this->sock_;
    this->sock_ = nullptr;
    return false;
  }

  bool Logger::emit(Message *msg, const std::string &tag, time_t ts) {
    if (this->msg_set_.find(msg) == this->msg_set_.end()) {
      this->errmsg_ = "invalid Message instance, "
        "should be got by Logger::retain_message()";
      return false;
    }

    if (!this->sock_ && !this->connect()) {
      return false;
    }

    this->msg_set_.erase(msg);

    if (ts == 0) {
      struct timeval tv;
      gettimeofday(&tv, nullptr);
      ts = tv.tv_sec;
    }

    msgpack::sbuffer buf;
    msgpack::packer <msgpack::sbuffer> pk (&buf);
    pk.pack_array(3);
    pk.pack(tag);
    pk.pack(ts);
    msg->to_msgpack(&pk);

    if (this->sock_) {
      this->sock_->send(buf.data(), buf.size());
    }

    delete msg;
    
    return true;
  }

}
