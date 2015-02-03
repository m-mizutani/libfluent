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
  EXPECT_EQ(res_tag, tag);
  EXPECT_EQ(res_rec, "{\"port\":443,\"url\":\"https://github.com\"}");
  delete e;
}


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
  EXPECT_EQ(res_rec, "{\"num\":0}");
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
  ASSERT_FALSE(get_line(&res_tag, &res_ts, &res_rec, 3));

  // Third emit should be fail because buffer is full.
  msg = new fluent::Message(tag);
  msg->set("num", 3);
  EXPECT_FALSE(e->emit(msg));
  ASSERT_FALSE(get_line(&res_tag, &res_ts, &res_rec, 3));

  delete e;  
}


