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

#ifndef __FLUENT_MESSAGE_HPP__
#define __FLUENT_MESSAGE_HPP__

#include <msgpack.hpp>
#include <iostream>
#include <map>
#include <assert.h>
#include "./exception.hpp"

namespace fluent {
  class Message {
  public:
    class Object;
    class Array;
    class Map;
    class String;
    class Fixnum;
    class Float;
    class Bool;
    
    Message(const std::string &tag);
    ~Message();
    
    // Set timestamp.
    void set_ts(time_t ts);
    time_t ts() const { return this->ts_; }
    const std::string& tag() const { return this->tag_; }

    // Set message data.
    bool set(const std::string &key, const std::string &val);
    bool set(const std::string &key, const char *val);
    bool set(const std::string &key, int val);
    bool set(const std::string &key, double val);
    bool set(const std::string &key, bool val);
    bool del(const std::string &key);
    Map *retain_map(const std::string &key);
    Array *retain_array(const std::string &key);
    const Object& get(const std::string &key) const {
      return this->root_->get(key);
    }

    // Convert to msgpack data format.
    void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    void to_ostream(std::ostream &os) const;
    friend std::ostream& operator<<(std::ostream& os, const Message& msg) {
      msg.to_ostream(os);
      return os;
    }
      
    // Linked list connect/disconnect.
    void attach(Message *next);
    Message* detach();
    Message* next() const { return this->next_; };
    Message* clone(Message *base=nullptr) const;

    class Object {
    public:
      Object() {}
      virtual ~Object() {}
      virtual void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
        const = 0;
      virtual void to_ostream(std::ostream &os) const = 0;
      
      virtual Object* clone() const = 0;
      virtual bool has_value() const { return true; }
      template <typename T> const T& as() const {
        const T* ptr = dynamic_cast<const T*>(this);
        if (ptr) {
          return *ptr;
        } else {
          std::string msg = "Can not convert to ";
          msg += typeid(T).name();
          throw Exception::TypeError(msg);
        }
      }
      template <typename T> bool is() const {
        const T* ptr = dynamic_cast<const T*>(this);
        return (ptr != nullptr);
      }
      
    };

    class Map : public Object {
    private:
      std::map<std::string, Object*> map_;
      static const bool DBG;
    public:
      Map();
      ~Map();
      Map *retain_map(const std::string &key);
      Array *retain_array(const std::string &key);
      bool set(const std::string &key, const std::string &val);
      bool set(const std::string &key, const char *val);
      bool set(const std::string &key, int val);
      bool set(const std::string &key, double val);
      bool set(const std::string &key, bool val);
      bool set(const std::string &key, Object *obj);
      bool del(const std::string &key);
      bool has_key(const std::string &key) const {
        return (this->map_.find(key) != this->map_.end());
      }
      const Object& get(const std::string &key) const;
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
      void to_ostream(std::ostream &os) const;      
      Object* clone() const;
    };

    class Array : public Object {
      std::vector<Object*> array_;
    public:
      Array() {}
      ~Array() {}
      Map *retain_map();
      Array *retain_array();
      void push(const std::string &val);
      void push(const char *val);
      void push(int val);
      void push(double val);
      void push(bool val);
      void push(Object *obj);
      size_t size() const { return this->array_.size(); }
      const Object& get(size_t idx) const;
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
      void to_ostream(std::ostream &os) const;      
      Object* clone() const;
    };

    class String : public Object {
    private:
      std::string val_;
    public:
      String(const std::string &val);
      String(const char *val);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
      void to_ostream(std::ostream &os) const {
        os << '"' << this->val_ << '"';
      }
      Object* clone() const { return new String(this->val_); }
      const std::string &val() const { return this->val_; }
    };

    class Fixnum : public Object {
    private:
      int val_;
    public:
      Fixnum(int val);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
      void to_ostream(std::ostream &os) const { os << this->val_; }
      Object* clone() const { return new Fixnum(this->val_); }
      int val() const { return this->val_; }
    };

    class Float : public Object {
    private:
      double val_;
    public:
      Float(double val);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
      void to_ostream(std::ostream &os) const { os << this->val_; }
      Object* clone() const { return new Float(this->val_); }
      double val() const { return this->val_; }
    };

    class Bool : public Object {
    private:
      bool val_;
    public:
      Bool(bool val);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
      void to_ostream(std::ostream &os) const { os << this->val_; }
      Object* clone() const { return new Bool(this->val_); }
      bool val() const { return this->val_; }
    };

  private:    
    time_t ts_;
    std::string tag_;
    Map *root_;
    Message *next_;    
  };
}


#endif   // __SRC_FLUENT_MESSAGE_H__
