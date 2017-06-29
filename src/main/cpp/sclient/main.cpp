#include<iostream>
#include "socket_client.hpp"
#include <chrono>
#include "utils.hpp"
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

using namespace asutils;

void configure_log4cpp() {

  std::string initFileName = "etc/log4cpp.properties";
  log4cpp::PropertyConfigurator::configure(initFileName);

}

void call_nblock(SocketClient &client, int32_t tc) {

  log4cpp::Category& logger = log4cpp::Category::getRoot();

  int32_t ip_calls = tc;

  std::mutex ipc_mutex;
  std::unique_lock<std::mutex> lck(ipc_mutex, std::defer_lock);
  std::condition_variable cv;

  uint64_t start = Utils::epoch_micros_now();

  for(int32_t i=0; i < tc; i++) {

    std::function<void(std::vector<char>)> cb = [&logger, &ip_calls, &ipc_mutex, &cv](std::vector<char> resp) {

      std::string rec_str(resp.begin(), resp.end());
      logger.info(std::string("Got back: ") + rec_str);

      //decrement counter
      ipc_mutex.lock();
      ip_calls--;
      ipc_mutex.unlock();
      cv.notify_one();

    };

    std::string hash_key = std::to_string(i);
    std::string msg = "blah blah";

    bool succss = client.send_msg((char*)msg.c_str(), msg.size(), hash_key, cb);
    
    if(!succss) {

      logger.error("Problem sending msg");

    } 
  
  }
  
  ipc_mutex.lock();
  while(ip_calls > 0) {

    cv.wait(lck);
    
  }
  ipc_mutex.unlock();

  uint64_t time = Utils::epoch_micros_now() - start;

  logger.info(std::string("This long to send: ") + std::to_string(time));

}

void call_block(SocketClient &client, int32_t tc) {
  
  log4cpp::Category& logger = log4cpp::Category::getRoot();

  //ThreadPool p_tp(std::thread::hardware_concurrency());
  ThreadPool p_tp(8); //if we use more threads then we block less.  leaving this as example

  int32_t ip_calls = tc;

  std::mutex ipc_mutex;
  std::unique_lock<std::mutex> lck(ipc_mutex, std::defer_lock);
  std::condition_variable cv;
 
  uint64_t start = Utils::epoch_micros_now();

  for(int32_t i=0; i < tc; i++) {

    std::function<void()> work = [&logger, i, &client, &ip_calls, &ipc_mutex, &cv]() {

      std::string hash_key = std::to_string(i);
      std::string msg = "blah blah";
      std::vector<char> resp;

      bool succss = client.send_msg((char*)msg.c_str(), msg.size(), hash_key, resp, (uint64_t)3);
      
      if(!succss) {

        logger.error("Problem sending msg.  Perhaps timeout");

      } else {

        std::string rec_str(resp.begin(), resp.end());
        logger.info(std::string("Got back: ") + rec_str);

      }

      //decrement counter
      ipc_mutex.lock();
      ip_calls--;
      ipc_mutex.unlock();
      cv.notify_one();
      
    };

    p_tp.add_work(work);
    
  }
  
  ipc_mutex.lock();
  while(ip_calls > 0) {

    cv.wait(lck);

  }
  ipc_mutex.unlock();

  uint64_t time = Utils::epoch_micros_now() - start;

  logger.info(std::string("This long to send: ") + std::to_string(time));

}

int main(int argc, char **argv) {

  configure_log4cpp();
	log4cpp::Category& logger = log4cpp::Category::getRoot();
  
  logger.info("Starting socket client");

  std::vector<std::string> hosts = {"localhost:8080", "localhost:8081"};
  SocketClient *client = new SocketClient(hosts);
  uint32_t c_conns = client->connect_to_hosts();

  if(c_conns == 0) {

    logger.error("Cannot connect to any of the clients yo!");
    exit(1);

  }

  int32_t tc = 1000000;

  call_block(*client, tc);
  //call_nblock(*client, tc);
  
}



