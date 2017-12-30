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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>


#include "./FluentTest.hpp"
#include "../src/fluent/logger.hpp"
#include "../src/fluent/message.hpp"
#include "../src/debug.h"


TEST_F(FluentTest, Logger) {
  fluent::Logger *logger = new fluent::Logger();
  logger->new_forward("localhost", 24224);
  
  const std::string tag = "test.http";
  fluent::Message *msg = logger->retain_message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);
  logger->emit(msg);
  std::string res_tag, res_ts, res_rec;
  get_line(&res_tag, &res_ts, &res_rec);
  EXPECT_EQ(res_tag, tag);
  EXPECT_EQ(res_rec, "{\"port\"=>443, \"url\"=>\"https://github.com\"}");

  delete logger;
}

TEST(Logger, textfile) {
  struct stat st;
  const std::string fname = "logger_test_output.txt";
  const std::string tag = "test.file";
  if (0 == ::stat(fname.c_str(), &st)) {
    ASSERT_TRUE(0 == unlink(fname.c_str()));
  }

  fluent::Logger *logger = new fluent::Logger();
  logger->new_textfile(fname);
  fluent::Message *msg = logger->retain_message(tag);
  msg->set("num", 1);
  msg->set_ts(1514633395);
  EXPECT_TRUE(logger->emit(msg));
  delete logger;

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


/*
 * Disabled because of unstable
 *
TEST_F(FluentTest, QueueLimit) {
  fluent::Logger *logger = new fluent::Logger();
  logger->new_forward("localhost", 24224);

  logger->set_queue_limit(1);
  const std::string tag = "test.http";

  fluent::Message *msg = logger->retain_message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);
  logger->emit(msg);
  
  std::string res_tag, res_ts, res_rec;
  get_line(&res_tag, &res_ts, &res_rec);
  EXPECT_EQ(res_tag, tag);
  EXPECT_EQ(res_rec, "{\"port\"=>443, \"url\"=>\"https://github.com\"}");
  this->stop_fluent();

  // First emit after stopping fluentd should be succeess because of buffer.
  msg = logger->retain_message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);
  EXPECT_TRUE(logger->emit(msg));

  // Second emit may be fail but do not test because of non blocking socket.
  msg = logger->retain_message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);
  logger->emit(msg);

  // Third emit should be fail because buffer is full.
  msg = logger->retain_message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);
  EXPECT_FALSE(logger->emit(msg));
  
  delete logger;
}
*/

TEST(Logger, QueueEmitter) {
  fluent::Logger *logger = new fluent::Logger();
  fluent::MsgQueue *q = logger->new_msgqueue();
  fluent::Message *msg = logger->retain_message("test.log");
  msg->set("race", "gnome");
  EXPECT_TRUE(logger->emit(msg));

  // Queue has the emitted message.
  msg = q->pop();
  EXPECT_TRUE(msg != nullptr);
  EXPECT_EQ(msg->get("race").as<fluent::Message::String>().val(), "gnome");

  // Queue has no more message.
  msg = q->pop();
  EXPECT_TRUE(msg == nullptr);
  
  delete logger;
}

TEST(Logger, TagPrefix) {
  fluent::Logger *logger = new fluent::Logger();
  fluent::Message* noprefix_msg = logger->retain_message("blue");
  EXPECT_EQ(noprefix_msg->tag(), "blue");

  logger->set_tag_prefix("dark");
  fluent::Message *withprefix_msg = logger->retain_message("blue");
  EXPECT_EQ(withprefix_msg->tag(), "dark.blue");
}
