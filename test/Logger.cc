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
  EXPECT_EQ(res_rec, "{\"port\":443,\"url\":\"https://github.com\"}");
}


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
  EXPECT_EQ(res_rec, "{\"port\":443,\"url\":\"https://github.com\"}");
  this->stop_fluent();

  // First emit after stopping fluentd should be succeess because of buffer.
  msg = logger->retain_message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);
  EXPECT_TRUE(logger->emit(msg));

  // Second emit should be fail because buffer is full.
  msg = logger->retain_message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);
  EXPECT_FALSE(logger->emit(msg));
}

