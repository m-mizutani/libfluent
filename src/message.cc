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

#include "./fluentd/message.hpp"
#include "./debug.h"

namespace fluentd {
  Message::Message() {
  };
  Message::~Message() {
  }

  Message::Map::Map() {
  }
  Message::Map::~Map() {
    for (auto it = this->map_.begin(); it != this->map_.end(); it++) {
      delete it->second;
    }
  }
  bool Message::Map::set(const std::string &key, int val) {
    if (this->map_.find(key) == this->map_.end()) {
      // Create and insert value
      Fixnum *n = new Fixnum(val);
      this->map_.insert(std::make_pair(key, n));
      return true;
    } else {
      // Already exists
      return false;
    }
  }
  void Message::Map::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
    debug(true, "to_msgpack");
    pk->pack_map(this->map_.size());
    for(auto it = this->map_.begin(); it != this->map_.end(); it++) {
      debug(true, "key: %s", (it->first).c_str());
      pk->pack(it->first);
      (it->second)->to_msgpack(pk);
    }
  }

  Message::Fixnum::Fixnum(int val) : val_(val) {}
  void Message::Fixnum::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
    const {
    pk->pack(this->val_);
  }  

}
