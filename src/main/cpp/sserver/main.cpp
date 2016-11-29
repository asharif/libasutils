#include<iostream>
#include "socket_server.hpp"
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

using namespace asutils;

void configure_log4cpp() {

  std::string initFileName = "etc/log4cpp.properties";
  log4cpp::PropertyConfigurator::configure(initFileName);

}

int main(int argc, char **argv) {

  configure_log4cpp();
  log4cpp::Category& logger = log4cpp::Category::getRoot();

  if(argc < 2 || argc > 2) {

    logger.error("Usage: server <port>");
    exit(1);
  }

  uint32_t port = std::stoi(argv[1]);

  logger.info("Starting socket server");

  std::function<void(std::vector<char> &&uuid_v, std::vector<char> &&msg_v, 
            SocketServer &server, const uint32_t sfd)> handler = [&logger](std::vector<char> &&uuid_v, std::vector<char> &&msg_v, 
            SocketServer &server, const uint32_t sfd) {

    std::string msg_s(msg_v.begin(), msg_v.end());

    logger.info(std::string("Got : ") + msg_s);

    std::string d = "no way bro";
    std::vector<char> n_msg(d.begin(), d.end());

    server.send_msg(uuid_v, n_msg, sfd);

  };

  SocketServer *server = new SocketServer(port, handler);
  delete server;

}
