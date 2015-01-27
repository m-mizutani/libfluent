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

#include "./fluent/message.hpp"
#include "./debug.h"

namespace fluent {
  Message::Message() {
  };
  Message::~Message() {
  }
  void Message::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
    this->root_.to_msgpack(pk);
  }

  
  const bool Message::Map::DBG(false);
  Message::Map::Map() {
  }
  Message::Map::~Map() {
    for (auto it = this->map_.begin(); it != this->map_.end(); it++) {
      delete it->second;
    }
  }

  // TODO: refactoring to merge set int, string, float, bool
  bool Message::Map::set(const std::string &key, int val) {
    if (this->map_.find(key) == this->map_.end()) {
      // Create and insert value
      Object *n = new Fixnum(val);
      this->map_.insert(std::make_pair(key, n));
      return true;
    } else {
      // Already exists
      return false;
    }
  }
  bool Message::Map::set(const std::string &key, const char *val) {
    if (this->map_.find(key) == this->map_.end()) {
      // Create and insert value
      debug(DBG, "create String object\n");

      Object *v = new String(val);
      this->map_.insert(std::make_pair(key, v));
      return true;
    } else {
      // Already exists
      return false;
    }
  }
  bool Message::Map::set(const std::string &key, const std::string &val) {
    if (this->map_.find(key) == this->map_.end()) {
      // Create and insert value
      debug(DBG, "create String object\n");

      Object *v = new String(val);
      this->map_.insert(std::make_pair(key, v));
      return true;
    } else {
      // Already exists
      return false;
    }
  }
  bool Message::Map::set(const std::string &key, double val) {

    if (this->map_.find(key) == this->map_.end()) {
      // Create and insert value
      debug(DBG, "create Double object for %s\n", key.c_str());
      Object *v = new Float(val);
      this->map_.insert(std::make_pair(key, v));
      return true;
    } else {
      // Already exists
      return false;
    }
  }
  bool Message::Map::set(const std::string &key, bool val) {

    if (this->map_.find(key) == this->map_.end()) {
      // Create and insert value
      debug(DBG, "create Bool object for %s\n", key.c_str());
      Object *v = new Bool(val);
      this->map_.insert(std::make_pair(key, v));
      return true;
    } else {
      // Already exists
      return false;
    }
  }

  void Message::Map::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk)
    const {
    pk->pack_map(this->map_.size());
    // Iterate all key and value to convert msgpack.
    for(auto it = this->map_.begin(); it != this->map_.end(); it++) {
      pk->pack(it->first);
      (it->second)->to_msgpack(pk);
    }
  }



  void Message::Array::push(const std::string &val) {
    Object *v = new String(val);
    this->array_.push_back(v);
  }
  void Message::Array::push(const char *val) {
    Object *v = new String(val);
    this->array_.push_back(v);
  }
  void Message::Array::push(int val) {
    Object *v = new Fixnum(val);
    this->array_.push_back(v);
  }
  void Message::Array::push(double val) {
    Object *v = new Float(val);
    this->array_.push_back(v);
  }
  void Message::Array::push(bool val) {
    Object *v = new Bool(val);
    this->array_.push_back(v);
  }

  void Message::Array::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
    pk->pack_array(this->array_.size());
    for(size_t i = 0; i < this->array_.size(); i++) {
      this->array_[i]->to_msgpack(pk);
    }
  }


  Message::Fixnum::Fixnum(int val) : val_(val) {}
  void Message::Fixnum::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
    const {
    pk->pack(this->val_);
  }  

  Message::String::String(const std::string &val) : val_(val) {}
  Message::String::String(const char *val) : val_(val) {}
  void Message::String::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
    const {
    pk->pack(this->val_);
  }

  Message::Float::Float(double val) : val_(val) {}
  void Message::Float::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
    const {
    pk->pack(this->val_);
  }  

  Message::Bool::Bool(bool val) : val_(val) {}
  void Message::Bool::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
    const {
    pk->pack(this->val_);
  }  
    
}
