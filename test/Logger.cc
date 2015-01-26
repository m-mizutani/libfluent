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

#include <regex>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>


#include "./gtest.h"
#include "../src/fluentd/logger.hpp"

#include "../src/debug.h"

class FluentdTest : public ::testing::Test {
private:
  enum {
    RP = 0, WP = 1
  };
  int pipe_fd_;
  pid_t pid_;
  
protected:
  FILE *pipe_;
  std::regex out_ptn_;

  
  FluentdTest() :
    pid_(0),
    out_ptn_("(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}) "
             "\\+\\d{4} (\\S+): (.*)")
  {
  }
  virtual ~FluentdTest() {
  }

  virtual void SetUp() {
    start_fluentd();
  }
  virtual void TearDown() {
    if (this->pid_ > 0) {
      stop_fluentd();
    }
  }

  void start_fluentd() {
    int pipe_c2p[2];
    ASSERT_TRUE(pipe(pipe_c2p) >= 0);
    
    pid_t pid = fork();
    if (pid == 0) {
      // Child
      /*
      char *const args[6] = {
        "fluentd",
        "-q", "--no-supervisor",
        "-c", "test/fluentd.conf", NULL};
      */
      std::vector<std::string> arg = {"fluentd",
                                      "-q", "--no-supervisor",
                                      "-c", "test/fluentd.conf"};
      char **argv = new char*[arg.size() + 1];
      for (size_t i = 0; i < arg.size(); i++) {
        argv[i] = const_cast<char*>(arg[i].c_str());
      }
      argv[arg.size()] = NULL;

      close(pipe_c2p[RP]);
      dup2(pipe_c2p[WP], 1);
      close(pipe_c2p[WP]);

      if (execvp("fluentd", static_cast<char *const *>(argv)) < 0) {
        perror("execvp");        
      }
    } else {
      this->pid_ = pid;
      this->pipe_fd_ = pipe_c2p[RP];
      this->pipe_ = fdopen(this->pipe_fd_, "r");
      ASSERT_TRUE(this->pipe_ != NULL);
    }
  }

  void stop_fluentd() {
    kill(this->pid_, SIGINT);
    int stat;
    int rc = waitpid(this->pid_, &stat, 0);
    ASSERT_EQ(this->pid_, rc);
    this->pid_ = 0;
    // EXPECT_TRUE(WIFSTOPPED(stat));
  }
  
  void get_line(std::string *tag, std::string *ts, std::string *rec) {
    char buf[BUFSIZ];
    EXPECT_TRUE(NULL != fgets(buf, BUFSIZ, this->pipe_));

    std::cmatch m;
    std::regex_search(buf, m, out_ptn_);
    ASSERT_EQ(m.size(), 4);
    *ts  = m[1];
    *tag = m[2];
    *rec = m[3];
  }

};

TEST_F(FluentdTest, Logger) {
  fluentd::Logger *logger = new fluentd::Logger();
  const size_t retry_max = 5;
  for (size_t i = 0; i < retry_max; i++) {
    if(logger->set_dest("localhost", "24224")) {
      break;
    } else {
      sleep(1);
    }
  }

  ASSERT_TRUE(logger->has_dest());
  
  fluentd::Message *msg = logger->retain_message();
  logger->emit(msg, "test.basic");
  std::string tag, ts, rec;
  get_line(&tag, &ts, &rec);
  EXPECT_EQ(tag, "test.basic");
  EXPECT_EQ(rec, "{}");
}

TEST(Logger, basic) {
  /*
  FILE *fp;
  char buf[BUFSIZ];
  std::string cmd("fluentd -q --no-supervisor -c test/fluentd.conf");

  fp = popen(cmd.c_str(), "r");

  if (fp == NULL) {
    std::cout << "ERROR: Logger test requires fluentd." << std::endl;
    std::cout << strerror(errno) << std::endl;
    ASSERT_TRUE(false);
  }


  fluentd::Logger *logger = new fluentd::Logger();
  const size_t retry_max = 5;
  for (size_t i = 0; i < retry_max; i++) {
    if(logger->set_dest("localhost", "24224")) {
      break;
    } else {
      sleep(1);
    }
  }

  EXPECT_TRUE(logger->has_dest());
  
  if (logger->has_dest()) {
    fluentd::Message *msg = logger->retain_message();
    logger->emit(msg, "test.basic");

    EXPECT_TRUE(NULL != fgets(buf, BUFSIZ, fp));
    std::cout << buf << std::endl;
  }
  pclose(fp);
  */
  
  /*
  while(fgets(buf, BUFSIZ, fp) != NULL) {
    fputs(buf, stdout);
  }
  */
  
}
