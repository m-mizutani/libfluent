#include <fluent.hpp>
	
int main(int argc, char *argv[]) {
  fluent::Logger *logger = new fluent::Logger();
  logger->new_forward("localhost", 24224);

  fluent::Message *msg = logger->retain_message("test.array");
  // {}
  fluent::Message::Array *arr = msg->retain_array("arr1");
  // {"arr1": []}
  arr->push(1);
  // {"arr1": [1]}
  arr->push(2);
  // {"arr1": [1, 2]}
  fluent::Message::Map *map = arr->retain_map();
  // {"arr1": [1, 2, {}]}
  map->set("t", "a");
  // {"arr1": [1, 2, {"t": "a"}]}
  
  logger->emit(msg);
  
  delete logger;
  return 0;
}
