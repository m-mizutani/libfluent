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

#include <string.h>
#include "./gtest.h"
#include "../src/fluentd/message.hpp"

TEST(Message, basic) {
  fluentd::Message::Map *obj = new fluentd::Message::Map();
  msgpack::sbuffer sbuf;
  msgpack::packer<msgpack::sbuffer> pkr(&sbuf);

  EXPECT_TRUE(obj->set("abc", 1));

  /*
    require 'msgpack'; a=[]; {'abc'=>1}.to_msgpack.each_byte{|v| 
    a.push(sprintf("0x%02X", v))};puts "{#{a.join(', ')}};"
  */
  uint8_t data[] = {0x81, 0xa3, 0x61, 0x62, 0x63, 0x01};
  obj->to_msgpack(&pkr);
  EXPECT_EQ(sbuf.size(), sizeof(data));
  EXPECT_TRUE(0 == memcmp(sbuf.data(), data, sizeof(data)));

  
}

TEST(Message, Map) {
  fluentd::Message::Map *obj = new fluentd::Message::Map();
  msgpack::sbuffer sbuf;
  msgpack::packer<msgpack::sbuffer> pkr(&sbuf);

  // Bool
  EXPECT_TRUE(obj->set("bool", true));
  // Float
  EXPECT_TRUE(obj->set("float", 34.567));
  // Fixnum
  EXPECT_TRUE(obj->set("int", 2345));
  // String
  EXPECT_TRUE(obj->set("str", "test"));

  /*
    d = {'bool'=>true, 'float'=>34.567, 'int'=>2345, 'str'=>'test', }
    require 'msgpack'; a=[]; d.to_msgpack.each_byte{|v| 
    a.push(sprintf("0x%02X", v))};puts "{#{a.join(', ')}};"
  */
  uint8_t expect[] = {0x84, 0xA4, 0x62, 0x6F, 0x6F, 0x6C, 0xC3, 0xA5,
                      0x66, 0x6C, 0x6F, 0x61, 0x74, 0xCB, 0x40, 0x41,
                      0x48, 0x93, 0x74, 0xBC, 0x6A, 0x7F, 0xA3, 0x69,
                      0x6E, 0x74, 0xCD, 0x09, 0x29, 0xA3, 0x73, 0x74,
                      0x72, 0xA4, 0x74, 0x65, 0x73, 0x74};

  obj->to_msgpack(&pkr);
  EXPECT_EQ(sbuf.size(), sizeof(expect));
  EXPECT_TRUE(0 == memcmp(sbuf.data(), expect, sizeof(expect)));
}

TEST(Message, Array) {
  fluentd::Message::Array *obj = new fluentd::Message::Array();
  msgpack::sbuffer sbuf;
  msgpack::packer<msgpack::sbuffer> pkr(&sbuf);

  // Bool
  obj->push(true);
  // Float
  obj->push(34.567);
  // Fixnum
  obj->push(2345);
  // String
  obj->push("test");

  /*
    d = [true, 34.567, 2345, 'test']
    require 'msgpack'; a=[]; d.to_msgpack.each_byte{|v| 
    a.push(sprintf("0x%02X", v))};puts "{#{a.join(', ')}};"
  */

  uint8_t expect[] = {0x94, 0xC3, 0xCB, 0x40, 0x41, 0x48, 0x93, 0x74,
                      0xBC, 0x6A, 0x7F, 0xCD, 0x09, 0x29, 0xA4, 0x74,
                      0x65, 0x73, 0x74};

  obj->to_msgpack(&pkr);
  EXPECT_EQ(sbuf.size(), sizeof(expect));
  EXPECT_TRUE(0 == memcmp(sbuf.data(), expect, sizeof(expect)));
}
