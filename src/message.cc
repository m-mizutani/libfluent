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

#include <time.h>
#include <assert.h>
#include "./fluent/message.hpp"
#include "./debug.h"

namespace fluent {
  Message::Message(const std::string &tag) :
    tag_(tag), root_(new Map()), next_(nullptr) {
    this->ts_ = time(nullptr);
  };
  Message::~Message() {
    delete this->root_;
    delete this->next_;
  }

  void Message::set_ts(time_t ts) {
    this->ts_ = ts;
  }
  
  bool Message::set(const std::string &key, const std::string &val) {
    return this->root_->set(key, val);
  }
  bool Message::set(const std::string &key, const char *val){
    return this->root_->set(key, val);
  }
  bool Message::set(const std::string &key, int val){
    return this->root_->set(key, val);
  }
  bool Message::set(const std::string &key, unsigned int val){
    return this->root_->set(key, val);
  }
  bool Message::set(const std::string &key, double val){
    return this->root_->set(key, val);
  }
  bool Message::set(const std::string &key, bool val){
    return this->root_->set(key, val);
  }
  bool Message::set_nil(const std::string &key){
    return this->root_->set_nil(key);
  }
  bool Message::del(const std::string &key){
    return this->root_->del(key);
  }
  Message::Map* Message::retain_map(const std::string &key) {
    return this->root_->retain_map(key);
  }
  Message::Array* Message::retain_array(const std::string &key) { 
    return this->root_->retain_array(key);
  }

  
  void Message::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
    pk->pack_array(3);          // [?, ?, ?]
    pk->pack(this->tag_);       // [tag, ?, ?]
    pk->pack(this->ts_);        // [tag, timestamp, ?]
    this->root_->to_msgpack(pk);
    return ;
  }
  void Message::to_ostream(std::ostream &os) const {
    struct tm time;
    gmtime_r(&(this->ts_), &time);
    char buf[128];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S+00:00", &time);

    os << buf << "\t" << this->tag_ << "\t";
    this->root_->to_ostream(os);
    os << "\n";
  }
  
  void Message::attach(Message *next) {
    assert(this->next_ == nullptr);
    this->next_ = next;
  }

  Message* Message::detach() {
    Message *m = this->next_;
    if (this->next_) {
      this->next_ = nullptr;
    }
    return m;
  }

  Message* Message::clone(Message *base) const {
    Message *msg = new Message(this->tag_);
    if (this->next_) {
      this->next_->clone(msg);
    }
    if (base) {
      base->attach(msg);
    }
    msg->set_ts(this->ts_);

    delete msg->root_;
    msg->root_ = dynamic_cast<Map*>(this->root_->clone());
    return msg;
  }


  
  const bool Message::Map::DBG(false);
  Message::Map::Map() {
  }
  Message::Map::~Map() {
    for (auto it = this->map_.begin(); it != this->map_.end(); it++) {
      delete it->second;
    }
  }

  Message::Map* Message::Map::retain_map(const std::string &key) {
    auto it = this->map_.find(key);
    if (it == this->map_.end()) {
      Map *obj = new Map();
      this->map_.insert(std::make_pair(key, obj));
      return obj;
    } else {
      if (it->second->is<Map>()) {
        return dynamic_cast<Map*>(it->second);
      } else {
        Map *obj = new Map();
        delete it->second;
        it->second = obj;
        return obj;
      }
    }
  }

  Message::Array* Message::Map::retain_array(const std::string &key) {
    auto it = this->map_.find(key);
    if (it == this->map_.end()) {
      Array *obj = new Array();
      this->map_.insert(std::make_pair(key, obj));
      return obj;
    } else {
      if (it->second->is<Array>()) {
        return dynamic_cast<Array*>(it->second);
      } else {
        Array *obj = new Array();
        delete it->second;
        it->second = obj;
        return obj;
      }
    }
  }
  
  // TODO: refactoring to merge set int, string, float, bool
  bool Message::Map::set(const std::string &key, int val) {
    Object *n = new Fixnum(val);
    return this->set(key, n);
  }
  bool Message::Map::set(const std::string &key, unsigned int val) {
    Object *n = new Ufixnum(val);
    return this->set(key, n);
  }  
  bool Message::Map::set(const std::string &key, const char *val) {
    Object *v = new String(val);
    return this->set(key, v);
  }
  bool Message::Map::set(const std::string &key, const std::string &val) {
    Object *v = new String(val);
    return this->set(key, v);
  }
  bool Message::Map::set(const std::string &key, double val) {
    Object *v = new Float(val);
    return this->set(key, v);
  }
  bool Message::Map::set(const std::string &key, bool val) {
    Object *v = new Bool(val);
    return this->set(key, v);
  }
  bool Message::Map::set(const std::string &key, Object *obj) {
    auto it = this->map_.find(key);

    // Allow overwrite
    if (it != this->map_.end()) {
      // Delete and put value
      delete it->second;
      it->second = obj;
    } else {
      // Create and insert value
      this->map_.insert(std::make_pair(key, obj));
    }
    
    return true;
  }

  bool Message::Map::set_nil(const std::string &key) {
    Nil *obj = new Nil();
    return this->set(key, obj);
  }
  
  bool Message::Map::del(const std::string &key) {
    auto it = this->map_.find(key);
    if (it != this->map_.end()) {
      // Create and insert value
      delete it->second;
      this->map_.erase(key);
      return true;
    } else {
      // Not exists.
      return false;
    }    
  }

  const Message::Object& Message::Map::get(const std::string &key) const {
    auto it = this->map_.find(key);
    if (it == this->map_.end()) {
      throw Exception::KeyError(key);
    } else {
      return *(it->second);
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

  void Message::Map::to_ostream(std::ostream &os) const {
    os << "{";
    if (this->map_.size() > 0) {
      const std::string &fkey = this->map_.begin()->first; // first key
      std::for_each(this->map_.begin(), this->map_.end(),
                    [&](std::pair <std::string, Object*> e) {
                      os << ((fkey != e.first) ? ", " : "")
                         << "\"" << e.first << "\": ";
                      (e.second)->to_ostream(os);
                    });
    }
    os << "}";
  }
  

  Message::Object* Message::Map::clone() const {
    Map *map = new Map();
    for(auto it = this->map_.begin(); it != this->map_.end(); it++) {
      map->set(it->first, (it->second)->clone());
    }
    return map;
  }

  Message::Array::~Array() {
    for (auto it : this->array_) {
      delete it;
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
  void Message::Array::push(unsigned int val) {
    Object *v = new Ufixnum(val);
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
  void Message::Array::push(Object *obj) {
    this->array_.push_back(obj);
  }
  void Message::Array::push_nil() {
    Object *v = new Nil();
    this->array_.push_back(v);
  }
  void Message::Array::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
    pk->pack_array(this->array_.size());
    for(size_t i = 0; i < this->array_.size(); i++) {
      this->array_[i]->to_msgpack(pk);
    }
  }

  void Message::Array::to_ostream(std::ostream &os) const {
    os << "[";
    if (this->array_.size() > 0) {
      Object *fobj = this->array_[0];
      std::for_each(this->array_.begin(), this->array_.end(),
                    [&](Object *obj) {
                      os << ((fobj != obj) ? ", " : "");
                      obj->to_ostream(os);
                    });
    }
    os << "]";
  }
  

  Message::Object* Message::Array::clone() const {
    Array *array = new Array();
    for(size_t i = 0; i < this->array_.size(); i++) {
      array->push(this->array_[i]->clone());
    }
    return array;
  }    
  
  Message::Map* Message::Array::retain_map() {
    Map *map = new Map();
    this->array_.push_back(map);
    return map;
  }
  Message::Array* Message::Array::retain_array() {
    Array *arr = new Array();
    this->array_.push_back(arr);
    return arr;
  }

  const Message::Object& Message::Array::get(size_t idx) const {
    if (idx < this->array_.size()) {
      return *(this->array_[idx]);
    } else {
      throw Exception::IndexError(idx, this->array_.size());
    }
  }


  Message::Fixnum::Fixnum(int val) : val_(val) {}
  void Message::Fixnum::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
    const {
    pk->pack(this->val_);
  }  

  Message::Ufixnum::Ufixnum(unsigned int val) : val_(val) {}
  void Message::Ufixnum::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
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

  void Message::Nil::to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const {
    pk->pack_nil();
  }
}
