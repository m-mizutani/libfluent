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
#include <map>

namespace fluent {
  class Message {
    
  public:
    Message(const std::string &tag);
    ~Message();
    void set_ts(time_t ts);
    time_t ts() const { return this->ts_; }
    const std::string& tag() const { return this->tag_; }
    
    void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    bool set(const std::string &key, const std::string &val);
    bool set(const std::string &key, const char *val);
    bool set(const std::string &key, int val);
    bool set(const std::string &key, double val);
    bool set(const std::string &key, bool val);
    bool del(const std::string &key);

    class Object {
    public:
      Object() {}
      virtual ~Object() {}
      virtual void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) 
        const = 0;
    };

    class Array;

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
      bool del(const std::string &key);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
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
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    };

    class String : public Object {
    private:
      std::string val_;
    public:
      String(const std::string &val);
      String(const char *val);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    };

    class Fixnum : public Object {
    private:
      int val_;
    public:
      Fixnum(int val);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    };

    class Float : public Object {
    private:
      double val_;
    public:
      Float(double val);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    };

    class Bool : public Object {
    private:
      bool val_;
    public:
      Bool(bool val);
      void to_msgpack(msgpack::packer<msgpack::sbuffer> *pk) const;
    };

  private:    
    time_t ts_;
    std::string tag_;
    Map root_;

  };
}


#endif   // __SRC_FLUENT_MESSAGE_H__
