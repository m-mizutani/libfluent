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

#include "./fluent/queue.hpp"
#include "./debug.h"

namespace fluent {
  // ----------------------------------------------------------------
  // MsgQueue
  const bool MsgQueue::DBG = false;
  
  MsgQueue::MsgQueue() :
    msg_head_(nullptr), msg_tail_(nullptr),
    count_(0), limit_(1000) {
  }
  MsgQueue::~MsgQueue() {
  }
  bool MsgQueue::push(Message *msg) {
    bool rc = true;

    if (this->count_ < this->limit_) {
      if (this->msg_head_) {
        assert(this->msg_tail_);
        this->msg_tail_->attach(msg);
      } else {
        this->msg_head_ = msg;
      }
      this->msg_tail_ = msg;
      this->count_++;
    } else {
      // queue is full.
      rc = false;
    }
    
    return rc;
  }
  
  Message* MsgQueue::pop() {
    Message *msg;
    
    if (this->msg_head_ == nullptr) {
      return nullptr;
    }

    assert(this->count_ > 0);
    msg = this->msg_head_;
    this->msg_head_ = this->msg_tail_ = nullptr;
    this->count_ = 0;
    
    return msg;
  }
  
  void MsgQueue::set_limit(size_t limit) {
    this->limit_ = limit;
  }


  // ----------------------------------------------
  const bool MsgThreadQueue::DBG = false;

  MsgThreadQueue::MsgThreadQueue() : term_(false) {
    // Setup pthread.
    ::pthread_mutex_init(&(this->mutex_), NULL);
    ::pthread_cond_init(&(this->cond_), NULL);
  }
  MsgThreadQueue::~MsgThreadQueue() {
  }
  bool MsgThreadQueue::push(Message *msg) {
    bool rc = true;
    
    ::pthread_mutex_lock (&(this->mutex_));
    debug(DBG, "entered lock");

    debug(DBG, "PUSH: count:%lu, limit:%lu", this->count(), this->limit());
    if (this->term_) {
      // do not accept more msg because working thread going to shutdown.
    } else {
      rc = this->MsgQueue::push(msg);
      ::pthread_cond_signal (&(this->cond_));
    }
    debug(DBG, "PUSHED: count:%lu, limit:%lu", this->count(), this->limit());
    
    ::pthread_mutex_unlock (&(this->mutex_));
    debug(DBG, "left lock");
    
    return rc;
  }
  Message* MsgThreadQueue::pop() {
    Message *msg;
    
    debug(DBG, "entering lock");
    ::pthread_mutex_lock(&(this->mutex_));
    debug(DBG, "entered lock");

    msg = this->MsgQueue::pop();
    if (msg != nullptr) {
      debug(DBG, "poped before wait (%p)", msg);
      ::pthread_mutex_unlock(&(this->mutex_));
      return msg;
    }

    if (this->term_) {
      // Going to shutdown the thread.
      ::pthread_mutex_unlock(&(this->mutex_));
      debug(DBG, "going to shutdown, leave");
      return nullptr;
    }
      
    debug(DBG, "entered wait");
    ::pthread_cond_wait(&(this->cond_), &(this->mutex_));
    debug(DBG, "left wait");

    msg = this->MsgQueue::pop();
    ::pthread_mutex_unlock(&(this->mutex_));
    if (msg) {
      debug(DBG, "poped (%p)", msg);
    } else {
      debug(DBG, "no data");
    }

    debug(DBG, "left lock");

    return msg;
  }

  void MsgThreadQueue::term() {
    // Sending terminate signal to worker thread.
    ::pthread_mutex_lock(&(this->mutex_));
    this->term_ = true;
    ::pthread_cond_signal (&(this->cond_));
    ::pthread_mutex_unlock(&(this->mutex_));    
    debug(DBG, "sent terminate");
  }

  bool MsgThreadQueue::is_term() {
    bool rc;
    ::pthread_mutex_lock(&(this->mutex_));
    rc = this->term_;
    ::pthread_mutex_unlock(&(this->mutex_));
    debug(DBG, "checked terminate");
    return rc;
  }
  
  void MsgThreadQueue::set_limit(size_t limit) {
    ::pthread_mutex_lock(&(this->mutex_));
    this->MsgQueue::set_limit(limit);
    ::pthread_mutex_unlock(&(this->mutex_));    
  }
  
}
