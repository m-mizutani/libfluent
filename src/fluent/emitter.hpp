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

#ifndef __FLUENT_EMITTER_HPP__
#define __FLUENT_EMITTER_HPP__

#include <string>
#include <pthread.h>
#include "./socket.hpp"
#include "./message.hpp"

namespace fluent {
  class MsgBuffer {
  private:
    std::deque<Message*> buf_;

  public:
    MsgBuffer();
    ~MsgBuffer();
    void push(Message *msg);
    Message *pull();
  };

  class MsgQueue {
  private:
    size_t buf_target_;
    MsgBuffer buf_[2];

  public:
    MsgQueue();
    ~MsgQueue();
    void push(Message *msg);
    MsgBuffer *switch_buffer();
  };

  class Emitter {
  private:
    static const int WAIT_MAX;

    Socket *sock_;
    pthread_t th_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    Message *msg_buf_;
    size_t retry_max_;
    std::string errmsg_;

    static void* run_thread(void *obj);
    void loop();
    bool connect();

  public:
    Emitter(const std::string &host, int port);
    ~Emitter();
    bool emit(Message *msg);
    const std::string& errmsg() const { return this->errmsg_; }
  };


}


#endif   // __SRC_FLUENT_EMITTER_H__
