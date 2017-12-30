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

// #include <regex>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>


#include "./gtest.h"
#include "./FluentTest.hpp"
#include "../src/fluent/emitter.hpp"

#include "../src/debug.h"


TEST_F(FluentTest, InetEmitter) {
  fluent::InetEmitter *e = new fluent::InetEmitter("localhost", 24224);
  const std::string tag = "test.inet";
  fluent::Message *msg = new fluent::Message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);

  // msg should be deleted by Emitter after sending
  e->emit(msg);
  
  std::string res_tag, res_ts, res_rec;
  EXPECT_TRUE(get_line(&res_tag, &res_ts, &res_rec));
  EXPECT_EQ(tag, res_tag);
  EXPECT_EQ("{\"port\"=>443, \"url\"=>\"https://github.com\"}", res_rec);
  delete e;
}

TEST_F(FluentTest, InetEmitter_with_string_portnum) {
  fluent::InetEmitter *e = new fluent::InetEmitter("localhost", "24224");
  const std::string tag = "test.inet";
  fluent::Message *msg = new fluent::Message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);

  // msg should be deleted by Emitter after sending
  e->emit(msg);
  
  std::string res_tag, res_ts, res_rec;
  EXPECT_TRUE(get_line(&res_tag, &res_ts, &res_rec));
  EXPECT_EQ(res_tag, tag);
  EXPECT_EQ(res_rec, "{\"port\"=>443, \"url\"=>\"https://github.com\"}");
  delete e;
}

/*
 * Disabled because of unstable interuction with other process
 *
TEST_F(FluentTest, InetEmitter_QueueLimit) {
  fluent::InetEmitter *e = new fluent::InetEmitter("localhost", 24224);
  std::string res_tag, res_ts, res_rec;
  e->set_queue_limit(1);
  const std::string tag = "test.inet";
  fluent::Message *msg = new fluent::Message(tag);
  msg->set("num", 0);
  // msg should be deleted by Emitter after sending
  e->emit(msg);
  
  ASSERT_TRUE(get_line(&res_tag, &res_ts, &res_rec));
  EXPECT_EQ(res_tag, tag);
  EXPECT_EQ(res_rec, "{\"num\"=>0}");
  this->stop_fluent();

  // First emit after stopping fluentd should be succeess
  // because of sending message.
  msg = new fluent::Message(tag);
  msg->set("num", 1);
  EXPECT_TRUE(e->emit(msg));
  ASSERT_FALSE(get_line(&res_tag, &res_ts, &res_rec, 3));

  // Second emit should be succeess because of storing message in buffer.
  msg = new fluent::Message(tag);
  msg->set("num", 2);
  EXPECT_TRUE(e->emit(msg));
  // ASSERT_FALSE(get_line(&res_tag, &res_ts, &res_rec, 3));

  // Third emit may be fail but do not test because of non block socket.
  msg = new fluent::Message(tag);
  msg->set("num", 3);
  e->emit(msg);

  // Forth emit should be fail because buffer is full.
  msg = new fluent::Message(tag);
  msg->set("num", 4);
  EXPECT_FALSE(e->emit(msg));
  // ASSERT_FALSE(get_line(&res_tag, &res_ts, &res_rec, 3));
  
  delete e;  
}
*/

TEST(FileEmitter, msgpack_mode) {
  struct stat st;
  const std::string fname = "fileemitter_test_output.msg";
  const std::string tag = "test.file";
  if (0 == ::stat(fname.c_str(), &st)) {
    ASSERT_TRUE(0 == unlink(fname.c_str()));
  }

  fluent::FileEmitter *e = new fluent::FileEmitter(fname);
  fluent::Message *msg = new fluent::Message(tag);
  msgpack::sbuffer sbuf;
  msgpack::packer<msgpack::sbuffer> pkr(&sbuf);
  
  msg->set("num", 1);
  msg->set_ts(100000);
  msg->to_msgpack(&pkr);
  EXPECT_TRUE(e->emit(msg));
  delete e;

  ASSERT_EQ (0, ::stat(fname.c_str(), &st));
  uint8_t buf[BUFSIZ];
  int fd = ::open(fname.c_str(), O_RDONLY);
  ASSERT_TRUE(fd > 0);
  int readsize = ::read(fd, buf, sizeof(buf));
  ASSERT_TRUE(readsize > 0);

  EXPECT_EQ(sbuf.size(), readsize);
  EXPECT_TRUE(0 == memcmp(sbuf.data(), buf, sbuf.size()));
  EXPECT_TRUE(0 == unlink(fname.c_str()));  
}

TEST(FileEmitter, text_mode) {
  struct stat st;
  const std::string fname = "fileemitter_test_output.txt";
  const std::string tag = "test.file";
  if (0 == ::stat(fname.c_str(), &st)) {
    ASSERT_TRUE(0 == unlink(fname.c_str()));
  }

  fluent::FileEmitter *e =
      new fluent::FileEmitter(fname, fluent::FileEmitter::Text);
  fluent::Message *msg = new fluent::Message(tag);
  msg->set("num", 1);
  msg->set_ts(1514633395);
  EXPECT_TRUE(e->emit(msg));
  delete e;

  std::string expected_text =
      "2017-12-30T11:29:55+00:00\ttest.file\t{\"num\": 1}\n";
  ASSERT_EQ (0, ::stat(fname.c_str(), &st));
  char buf[BUFSIZ];
  int fd = ::open(fname.c_str(), O_RDONLY);
  ASSERT_TRUE(fd > 0);
  int readsize = ::read(fd, buf, sizeof(buf));
  ASSERT_TRUE(readsize > 0);

  std::string text(buf, readsize);
  EXPECT_EQ(expected_text, text);
  EXPECT_TRUE(0 == unlink(fname.c_str()));
}



TEST(QueueEmitter, basic) {
  const std::string tag = "test.queue";
  fluent::MsgQueue *q = new fluent::MsgQueue();
  fluent::QueueEmitter *qe = new fluent::QueueEmitter(q);

  fluent::Message *msg1 = new fluent::Message(tag);
  msg1->set("seq", 1);
  EXPECT_TRUE(nullptr == q->pop());
  // Emit (store the message)
  EXPECT_TRUE(qe->emit(msg1));

  fluent::Message *msg2 = new fluent::Message(tag);
  msg2->set("seq", 2);
  // Emit (store the message)
  EXPECT_TRUE(qe->emit(msg2));

  
  // Fetch the stored message.
  fluent::Message *pmsg;

  // First message
  pmsg = q->pop();
  ASSERT_TRUE(nullptr != pmsg);
  const fluent::Message::Fixnum &fn1 =
    pmsg->get("seq").as<fluent::Message::Fixnum>();
  EXPECT_EQ(1, fn1.val());
  delete pmsg;

  // Second message
  pmsg = q->pop();
  ASSERT_TRUE(nullptr != pmsg);
  const fluent::Message::Fixnum &fn2 =
    pmsg->get("seq").as<fluent::Message::Fixnum>();
  EXPECT_EQ(2, fn2.val());
  delete pmsg;

  // No messages.
  EXPECT_TRUE(nullptr == q->pop());
}
