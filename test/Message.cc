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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "./gtest.h"
#include "../src/fluent/message.hpp"
#include "../src/fluent/exception.hpp"
#include "../src/debug.h"

class MessageTest : public ::testing::Test {
private:
  enum { RP = 0, WP = 1 };
  
protected:
  msgpack::sbuffer sbuf_;
  msgpack::packer<msgpack::sbuffer> pkr_;
  MessageTest() : pkr_(&(this->sbuf_)) {}
  virtual ~MessageTest() {}
  virtual void SetUp() {}
  virtual void TearDown() {}

  std::string encode_msgpack(const std::string ruby_code) {
    std::stringstream ss;
    ss << "d=" << ruby_code << ";" <<
      "require 'msgpack'; STDOUT.write(d.to_msgpack) ";
    
    int pipe_c2p[2], pipe_p2c[2];
    pipe(pipe_c2p);
    pipe(pipe_p2c);
    
    pid_t pid = fork();
    if (pid == 0) {
      // Running as child.
      std::vector<std::string> arg = {"ruby"};
      char **argv = new char*[arg.size() + 1];
      for (size_t i = 0; i < arg.size(); i++) {
        argv[i] = const_cast<char*>(arg[i].c_str());
      }
      argv[arg.size()] = NULL;

      close(pipe_p2c[WP]);
      close(pipe_c2p[RP]);
      
      dup2(pipe_p2c[RP], 0);
      dup2(pipe_c2p[WP], 1);
      close(pipe_p2c[RP]);
      close(pipe_c2p[WP]);

      if (execvp("ruby", static_cast<char *const *>(argv)) < 0) {
        perror("execvp"); 
      }
    }

    // Running as parent.
    write(pipe_p2c[WP], ss.str().c_str(), ss.str().length());
    close(pipe_p2c[WP]);
    char buf[BUFSIZ];
    int stat;
    int rsize = read(pipe_c2p[RP], buf, BUFSIZ);
    int rc = waitpid(pid, &stat, 0);
    EXPECT_EQ(pid, rc);

    std::string res(buf, rsize);
    return res;
  }
};

TEST(Message, basic) {
  fluent::Message::Map *obj = new fluent::Message::Map();
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

TEST_F(MessageTest, Map) {
  fluent::Message::Map *obj = new fluent::Message::Map();
  // Bool
  EXPECT_TRUE(obj->set("bool", true));
  // Float
  EXPECT_TRUE(obj->set("float", 34.567));
  // Fixnum
  EXPECT_TRUE(obj->set("int", 2345));
  // String
  EXPECT_TRUE(obj->set("str", "test"));

  std::string expect = encode_msgpack("{'bool'=>true, 'float'=>34.567, "
                                      "'int'=>2345, 'str'=>'test', }");
  obj->to_msgpack(&pkr_);
  EXPECT_EQ(sbuf_.size(), expect.length());
  EXPECT_TRUE(0 == memcmp(sbuf_.data(), expect.data(), sbuf_.size()));
}

TEST_F(MessageTest, Array) {
  fluent::Message::Array *obj = new fluent::Message::Array();
  // Bool
  obj->push(true);
  // Float
  obj->push(34.567);
  // Fixnum
  obj->push(2345);
  // String
  obj->push("test");

  std::string expect = encode_msgpack("[true, 34.567, 2345, 'test']");
  obj->to_msgpack(&pkr_);
  EXPECT_EQ(sbuf_.size(), expect.length());
  EXPECT_TRUE(0 == memcmp(sbuf_.data(), expect.data(), sbuf_.size()));
}

TEST_F(MessageTest, NestArray) {
  fluent::Message::Array *obj = new fluent::Message::Array();
  obj->push("a");  // ["a"]
  fluent::Message::Array *arr = obj->retain_array(); // ["a", []]
  ASSERT_TRUE(arr != nullptr);
  arr->push("b"); // ["a", ["b"]]
  arr->push(1);   // ["a", ["b", 1]]
  fluent::Message::Map *map = obj->retain_map();  // ["a", ["b", 1], {}]
  ASSERT_TRUE(map != nullptr);
  map->set("c", 1); // ["a", ["b", 1], {"c": 1}}]

  std::string expect = encode_msgpack("['a',['b', 1],{'c'=>1}]");
  obj->to_msgpack(&pkr_);
  EXPECT_EQ(sbuf_.size(), expect.length());
  EXPECT_TRUE(0 == memcmp(sbuf_.data(), expect.data(), sbuf_.size()));
}

TEST_F(MessageTest, NestMap) {
  fluent::Message::Map *obj = new fluent::Message::Map();
  
  obj->set("a", 1);  // {"a":1}
  fluent::Message::Array *arr = obj->retain_array("b"); // {"a":1, "b": []}
  ASSERT_TRUE(arr != nullptr);
  arr->push(2); // {"a":1, "b": [2]}
  arr->push(3); // {"a":1, "b": [2, 3]}
  fluent::Message::Map *map = obj->retain_map("c");
  // {"a":1, "b": [2, 3], "c": {}}
  ASSERT_TRUE(map != nullptr);
  map->set("d", 4);   // {"a":1, "b": [2, 3], "c": {"d":4}}

  std::string expect = encode_msgpack("{'a'=>1, 'b'=>[2,3], 'c'=>{'d'=>4}}");
  obj->to_msgpack(&pkr_);
  EXPECT_EQ(sbuf_.size(), expect.length());
  EXPECT_TRUE(0 == memcmp(sbuf_.data(), expect.data(), sbuf_.size()));
}

TEST_F(MessageTest, NotOverwriteMap) {
  fluent::Message::Map *obj = new fluent::Message::Map();

  EXPECT_TRUE(obj->set("a", 1));
  EXPECT_FALSE(obj->set("a", 2));

  std::string expect = encode_msgpack("{'a'=>1}");

  obj->to_msgpack(&pkr_);
  EXPECT_EQ(sbuf_.size(), expect.length());
  EXPECT_TRUE(0 == memcmp(sbuf_.data(), expect.data(), sbuf_.size()));
}

TEST(Message, link) {
  // TODO: add tests
}

TEST(Message, MapGetObject) {
  fluent::Message::Map *obj = new fluent::Message::Map();
  obj->set("i", 1);
  obj->set("s", "test");
  obj->set("f", 3.141592);
  obj->set("b", true);
  fluent::Message::Map *map = obj->retain_map("m");
  map->set("gnome", 1);
  fluent::Message::Array *arr = obj->retain_array("a");
  arr->push("druid");
  
  // Check Key.
  EXPECT_TRUE(obj->has_key("i"));
  EXPECT_TRUE(obj->has_key("s"));
  EXPECT_TRUE(obj->has_key("f"));
  EXPECT_TRUE(obj->has_key("b"));
  EXPECT_TRUE(obj->has_key("m"));
  EXPECT_TRUE(obj->has_key("a"));
  EXPECT_FALSE(obj->has_key("x"));

  // Get objects.
  const fluent::Message::Object &obj_i = obj->get("i");
  const fluent::Message::Object &obj_s = obj->get("s");
  const fluent::Message::Object &obj_f = obj->get("f");
  const fluent::Message::Object &obj_b = obj->get("b");
  const fluent::Message::Object &obj_m = obj->get("m");
  const fluent::Message::Object &obj_a = obj->get("a");
  EXPECT_THROW(obj->get("x"), fluent::Exception::KeyError);  

  // Check types.
  EXPECT_TRUE(obj_i.is<fluent::Message::Fixnum>());
  EXPECT_TRUE(obj_s.is<fluent::Message::String>());
  EXPECT_TRUE(obj_f.is<fluent::Message::Float>());
  EXPECT_TRUE(obj_b.is<fluent::Message::Bool>());
  EXPECT_TRUE(obj_m.is<fluent::Message::Map>());
  EXPECT_TRUE(obj_a.is<fluent::Message::Array>());
  
  // Convert to appropriate type.
  const fluent::Message::Fixnum &i = obj_i.as<fluent::Message::Fixnum>();
  const fluent::Message::String &s = obj_s.as<fluent::Message::String>();
  const fluent::Message::Float &f = obj_f.as<fluent::Message::Float>();
  const fluent::Message::Bool &b = obj_b.as<fluent::Message::Bool>();
  const fluent::Message::Map &m = obj_m.as<fluent::Message::Map>();
  const fluent::Message::Array &a = obj_a.as<fluent::Message::Array>();
  EXPECT_THROW(obj_i.as<fluent::Message::String>(),
               fluent::Exception::TypeError);
  EXPECT_THROW(obj_i.as<fluent::Message::Float>(),
               fluent::Exception::TypeError);

  // Check values.
  EXPECT_EQ(i.val(), 1);
  EXPECT_EQ(s.val(), "test");
  EXPECT_EQ(f.val(), 3.141592);
  EXPECT_EQ(b.val(), true);
  
  EXPECT_TRUE(m.has_key("gnome"));
  EXPECT_FALSE(m.has_key("x"));
  EXPECT_EQ(m.get("gnome").as<fluent::Message::Fixnum>().val(), 1);
  EXPECT_EQ(a.size(), 1);
  EXPECT_EQ(a.get(0).as<fluent::Message::String>().val(), "druid");

  EXPECT_THROW(a.get(1), fluent::Exception::IndexError);
}
