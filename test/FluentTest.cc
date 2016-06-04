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

#include "./FluentTest.hpp"
#include <unistd.h>

FluentTest::FluentTest() : pid_(0) {
}

FluentTest::~FluentTest() {
}

void FluentTest::SetUp() {
  start_fluent();
}

void FluentTest::TearDown() {
  if (this->pid_ > 0) {
    stop_fluent();
  }
}

void FluentTest::start_fluent() {
  int pipe_c2p[2];
  ASSERT_TRUE(pipe(pipe_c2p) >= 0);
    
  pid_t pid = fork();
  if (pid == 0) {
    // Running as child.
    std::vector<std::string> arg = {
      "fluentd", "-q", "-c", "test/fluentd.conf"
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

    int flags = fcntl(pipe_c2p[RP], F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(pipe_c2p[RP], F_SETFL, flags);
      
      
    this->pipe_fd_ = pipe_c2p[RP];
    this->pipe_ = fdopen(this->pipe_fd_, "r");
    ASSERT_TRUE(this->pipe_ != NULL);
  }
}

void FluentTest::stop_fluent() {
  kill(this->pid_, SIGINT);
  int stat;
  int rc = waitpid(this->pid_, &stat, 0);
  ASSERT_EQ(this->pid_, rc);
  this->pid_ = 0;
  close(this->pipe_fd_);
}
  
bool FluentTest::get_line(std::string *tag, std::string *ts, std::string *rec,
                          time_t timeout) {
  static const bool DBG = false;
  char buf[BUFSIZ];

  sleep(1);
  time_t start_ts = time(nullptr);
  char *res;
  while (nullptr == (res = fgets(buf, BUFSIZ, this->pipe_))) {
    usleep(100000);
    if (time(nullptr) - start_ts >= timeout) {
      return false;
    }
  }
    
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

  ts->assign(line, 0, tag_ptr);
  tag->assign(line, tag_ptr + 1, rec_ptr - tag_ptr - 2);
  rec->assign(line, rec_ptr + 1, line.length() - rec_ptr - 2);
  return true;
}


