libfluent
==============

Fluentd library in C++. The library makes C++ program available to send logs 
to fluentd daemon directly.

```c++
#include <fluent.hpp>
	
int main(int argc, char *argv[]) {
  fluent::Logger *logger = new fluent::Logger("localhost", 24224);
	  
  // Emit ["tag.http", 1422315xxx, {"url": "http://github.com", "port": 443}]
  fluent::Message *msg = logger->retain_message("tag.http");
  msg->set("url", "http://github.com");
  msg->set("port", 443);
  logger->emit(msg);
  
  delete logger;
}
```


Prerequisite
--------------

- C++11
- libmsgpack
- fluentd (for test)

Functions
--------------

- Support nested message such as `{"a": {"b": {"c": 3}}}`
- Support array in message such as `{"a": [1, 2, 3]}`
- Support multithread buffering (under development)
- Reconnect when disconnected (under development)
- Exponential backoff for reconnect

Author
--------------
- Masayoshi Mizutani <mizutani@sfc.wide.ad.jp>
