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
  const int Logger::WAIT_MAX = 120 * 1000;
  Logger::Logger(const std::string &host, int port) :
    host_(host),
    port_(port),
    retry_max_(20)
  {
    this->emitter_ = new InetEmitter(host, port);
  }
  Logger::~Logger() {
    // delete not used messages.
    for (auto it = this->msg_set_.begin(); it != this->msg_set_.end(); it++) {
      delete *it;
    }
  }

  Message* Logger::retain_message(const std::string &tag) {
    Message *msg = new Message(tag);
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
    return this->emitter_->emit(msg);
  }

  void Logger::set_queue_limit(size_t limit) {
    this->emitter_->set_queue_limit(limit);
  }

}
