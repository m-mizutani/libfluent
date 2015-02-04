#include <fluent.hpp>
	
int main(int argc, char *argv[]) {
  fluent::Logger *logger = new fluent::Logger();
  logger->new_forward("localhost", 24224);
	  
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
  
  logger->emit(msg);
  
  delete logger;
  return 0;
}
