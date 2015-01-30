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


#include "./gtest.h"
#include "../src/fluent/logger.hpp"
#include "../src/fluent/message.hpp"

#include "../src/debug.h"

class FluentTest : public ::testing::Test {
private:
  enum {
    RP = 0, WP = 1
  };
  int pipe_fd_;
  pid_t pid_;
  
protected:
  FILE *pipe_;
  // std::regex out_ptn_;

  
  FluentTest() :
    pid_(0)
    /*
    out_ptn_("(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}) "
             "\\+\\d{4} (\\S+): (.*)")
    */
  {
  }
  virtual ~FluentTest() {
  }

  virtual void SetUp() {
    start_fluent();
  }
  virtual void TearDown() {
    if (this->pid_ > 0) {
      stop_fluent();
    }
  }

  void start_fluent() {
    int pipe_c2p[2];
    ASSERT_TRUE(pipe(pipe_c2p) >= 0);
    
    pid_t pid = fork();
    if (pid == 0) {
      // Running as child.
      std::vector<std::string> arg = {
        "fluentd", "-q", "--no-supervisor", "-c", "test/fluentd.conf"
      };
        
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
      // Running as parent.
      this->pid_ = pid;
      this->pipe_fd_ = pipe_c2p[RP];
      this->pipe_ = fdopen(this->pipe_fd_, "r");
      ASSERT_TRUE(this->pipe_ != NULL);
    }
  }

  void stop_fluent() {
    kill(this->pid_, SIGINT);
    int stat;
    int rc = waitpid(this->pid_, &stat, 0);
    ASSERT_EQ(this->pid_, rc);
    this->pid_ = 0;
    close(this->pipe_fd_);
    // EXPECT_TRUE(WIFSTOPPED(stat));
  }
  
  void get_line(std::string *tag, std::string *ts, std::string *rec) {
    static const bool DBG = false;
    char buf[BUFSIZ];
    EXPECT_TRUE(NULL != fgets(buf, BUFSIZ, this->pipe_));
    debug(false, "buf = %s", buf);
    std::string line(buf);

    size_t date_ptr = line.find(" ");
    size_t time_ptr = line.find(" ", date_ptr + 1);
    size_t tag_ptr  = line.find(" ", time_ptr + 1);
    size_t rec_ptr  = line.find(" ", tag_ptr  + 1);

    debug(DBG, "date_ptr = %lu", date_ptr);
    debug(DBG, "time_ptr = %lu", time_ptr);
    debug(DBG, "tag_ptr  = %lu", tag_ptr);
    debug(DBG, "rec_ptr  = %lu", rec_ptr);

    // std::cmatch m;
    // std::regex_search(buf, m, out_ptn_);
    // ASSERT_EQ(m.size(), 4);
    ts->assign(line, 0, tag_ptr);
    tag->assign(line, tag_ptr + 1, rec_ptr - tag_ptr - 2);
    rec->assign(line, rec_ptr + 1, line.length() - rec_ptr - 2);
  }

};

TEST_F(FluentTest, Logger) {
  fluent::Logger *logger = new fluent::Logger("localhost", 24224);
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

/*
TEST_F(FluentTest, Fail) {
  fluent::Logger *logger = new fluent::Logger("localhost", 24224);
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

  msg = logger->retain_message(tag);
  msg->set("url", "https://github.com");
  msg->set("port", 443);
  EXPECT_FALSE(logger->emit(msg));

}
*/

TEST(Logger, basic) {
  /*
  FILE *fp;
  char buf[BUFSIZ];
  std::string cmd("fluent -q --no-supervisor -c test/fluent.conf");

  fp = popen(cmd.c_str(), "r");

  if (fp == NULL) {
    std::cout << "ERROR: Logger test requires fluent." << std::endl;
    std::cout << strerror(errno) << std::endl;
    ASSERT_TRUE(false);
  }


  fluent::Logger *logger = new fluent::Logger();
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
    fluent::Message *msg = logger->retain_message();
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
