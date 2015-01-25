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

#ifndef __FLUENTD_MESSAGE_HPP__
#define __FLUENTD_MESSAGE_HPP__

#include <msgpack.hpp>

namespace fluentd {
  class Message {
  public:
    Message();
    ~Message();

    class Object {
    public:
      virtual void to_msgpack(msgpack::sbuffer *sbuf) = 0;
    };

    class Array;

    class Map : public Object {
    public:
      Map() {};
      ~Map() {};
      Map *retain_map(const std::string &key);
      Array *retain_array(const std::string &key);
      void put(const std::string &key, const std::string &val);
      void put(const std::string &key, int val);
      void put(const std::string &key, float val);
      void put(const std::string &key, bool val);
      void to_msgpack(msgpack::sbuffer *sbuf);
    };

    class Array : public Object {
    public:
      Map *retain_map();
      Array *retain_array();
      void put(const std::string &key, const std::string &val);
      void put(const std::string &key, int val);
      void put(const std::string &key, float val);
      void put(const std::string &key, bool val);
      void to_msgpack(msgpack::sbuffer *sbuf);
    };

    class String : public Object {
    public:
      String(const std::string &val);
      void to_msgpack(msgpack::sbuffer *sbuf);    
    };

    class Fixnum : public Object {
    public:
      Fixnum(int val);
      void to_msgpack(msgpack::sbuffer *sbuf);
    };

    class Float : public Object {
    public:
      Float(float val);
      void to_msgpack(msgpack::sbuffer *sbuf);
    };

  };

}


#endif   // __SRC_FLUENTD_MESSAGE_H__
