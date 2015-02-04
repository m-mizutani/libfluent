#include <fluent.hpp>
	
int main(int argc, char *argv[]) {
  fluent::Logger *logger = new fluent::Logger();
  logger->new_forward("localhost", 24224);
	  
  // Emit log:
  //  tag: "test.http"
  //  time: 1422315xxx (auto)
  //  data: {"url": "http://github.com", "port": 443}
  fluent::Message *msg = logger->retain_message("test.http");
  msg->set("url", "http://github.com");
  msg->set("port", 443);
  logger->emit(msg);
  
  delete logger;
}
