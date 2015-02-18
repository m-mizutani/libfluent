libfluent
==============

Fluentd library in C++. The library makes C++ program available to send logs 
to fluentd daemon directly.

```c++
#include <fluent.hpp>
	
int main(int argc, char *argv[]) {
  fluent::Logger *logger = new fluent::Logger();
  logger->new_forward("localhost", 24224);
	  
  // Emit log:
  //  tag: "tag.http"
  //  time: 1422315xxx (auto)
  //  data: {"url": "http://github.com", "port": 443}
  fluent::Message *msg = logger->retain_message("tag.http");
  msg->set("url", "http://github.com");
  msg->set("port", 443);
  logger->emit(msg);
  
  delete logger;
}
```

Functions
--------------

- Support nested message such as `{"a": {"b": {"c": 3}}}`
- Support array in message such as `{"a": [1, 2, 3]}`
- Asynchronous emitting and buffering
- Reconnect when disconnected
- Exponential backoff for reconnect


Prerequisite
--------------

- C++11 compiler
- libmsgpack >= 0.5.9
- ruby, fluentd, msgpack-ruby (for test)

Install
--------------

```shell
% cmake .
% make
% sudo make install
```

Examples
--------------

### Nested Array/Map

Map
```c++
fluent::Message *msg = logger->retain_message("test.map");
// {}
msg->set("t1", "a");
// {"t1": "a"}
fluent::Message::Map *m1 = msg->retain_map("map1");
// {"t1": "a", "map1": {}}
m1->set("t2", "b");
// {"t2": "a", "map1": {"t2": "b"}}
fluent::Message::Map *m2 = m1->retain_map("map2");
// {"t1": "a", "map1": {"t2": "b", "map2":{}}}
m2->set("t2", "b");
// {"t2": "a", "map1": {"t2": "b", "map2":{"t2": "b"}}}
```

Array
```c++
fluent::Message *msg = logger->retain_message("test.array");
// {}
fluent::Message::Array *arr = msg->retain_array("arr1");
// {"arr1": []}
arr->push(1);
// {"arr1": [1]}
arr->push(2);
// {"arr1": [1, 2]}
fluent::Message::Map *map = arr->retain_map("map2");
// {"arr1": [1, 2, {}]}
map->set("t", "a");
// {"arr1": [1, 2, {"t": "a"}]}
```

Author
--------------
- Masayoshi Mizutani <mizutani@sfc.wide.ad.jp>
