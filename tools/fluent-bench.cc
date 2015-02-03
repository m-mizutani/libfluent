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

#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include "../src/fluent.hpp"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "syntax) fluent-bench <host> <port> <msg/sec>" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string host(argv[1]);
  int port = std::stoi(argv[2]);
  int mps  = std::stoi(argv[3]);

  fluent::Logger *logger = new fluent::Logger();
  logger->new_forward(host, port);

  int wait = 1000000 / mps;
  time_t last_ts = time(nullptr);
  size_t msg_count = 0;

  std::cout << "wait: " << wait << "usec" << std::endl;
  
  while (true) {
    fluent::Message *msg = logger->retain_message("test.bench");
    msg->set("this", "test");
    if (!logger->emit(msg)) {
      std::cerr << "emit error: " << logger->errmsg() << std::endl;
      break;
    }
    msg_count++;
    
    time_t now_ts = time(nullptr);
    if (now_ts > last_ts) {
      last_ts = now_ts;
      std::cout << last_ts << ": " << msg_count << " mps" << std::endl;
      // double mps_real   = 1000000 / static_cast<double>(msg_count);
      // double mps_adjust = mps_target / msg_real;
      int diff = (mps > msg_count) ? mps - msg_count : msg_count - mps;
      double ratio = static_cast<double>(diff) / static_cast<double>(mps);
      int adjust = static_cast<int>(static_cast<double>(wait) * ratio);
      if (adjust == 0) {
        adjust = 1;
      }
      
      if (mps < msg_count) {
        wait += adjust;
      } else if (mps > msg_count) {
        wait -= adjust;
        if (wait <= 0) {
          wait = 1;
        }
      }      
      msg_count = 0;
    }
    usleep(wait);
  }
  return 0;
}
