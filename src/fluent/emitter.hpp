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
#include "./queue.hpp"

namespace fluent {
  class Emitter {
  private:
    pthread_t th_;
    std::string errmsg_;
    
    static void* run_thread(void *obj);
    virtual void worker() = 0;
    
  protected:
    const bool DBG = true;
    MsgThreadQueue queue_;
    void set_errmsg(const std::string &errmsg) {
      this->errmsg_ = errmsg;
    }
    void start_worker();
    void stop_worker();

  public:
    Emitter();
    virtual ~Emitter();
    void set_queue_limit(size_t limit);
    virtual bool emit(Message *msg);
    const std::string& errmsg() const { return this->errmsg_; }
  };

  class InetEmitter : public Emitter {
  private:
    static const int WAIT_MAX;

    void init(const std::string &host, const std::string &port);

    std::random_device random_device;
    std::mt19937 mt_rand;
    std::uniform_int_distribution<int> rand_dist;

    Socket *sock_;
    size_t retry_limit_;
    bool connect();

  public:
    InetEmitter(const std::string &host, int port);
    InetEmitter(const std::string &host, const std::string &port);
    ~InetEmitter();
    void worker();
  };

  class FileEmitter : public Emitter {
  private:
    int fd_;
    bool enabled_;
    bool opened_;
  public:
    FileEmitter(const std::string &fname);
    FileEmitter(int fd);
    ~FileEmitter();
    void worker();
  };

  class QueueEmitter : public Emitter {
  private:
    MsgQueue *q_;
  public:
    QueueEmitter(MsgQueue *q);
    ~QueueEmitter();
    void worker();
    bool emit(Message *msg);    
  };
  
}


#endif   // __SRC_FLUENT_EMITTER_H__
