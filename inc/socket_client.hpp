#ifndef AS_UTILS_SOCKET_CLIENT_HPP
#define AS_UTILS_SOCKET_CLIENT_HPP

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <vector>
#include <mutex>
#include <thread>
#include <fcntl.h>
#include <condition_variable>
#include "socket_utils.hpp" 
#include "thread_pool.hpp"
#include <unordered_map>
#include <exception>
#include "utils.hpp" 
#include "buffered_reader.hpp"
#include "buffered_writer.hpp"
#include <csignal>
#include <chrono>
#include <atomic>
#include <log4cpp/Category.hh>

namespace asutils {

  class SocketClient {
		
    private:

      struct HostStatus {

        bool is_healthy;
        int32_t sfd;
        int32_t ep_sfd;

      };

      static log4cpp::Category &logger;

      /**
       * Threadpool for reading data
       */
      ThreadPool r_tp;

      /**
       * Threadpool for writing data
       */
      ThreadPool w_tp;

      /**
       * The desired hosts
       */
      std::vector<std::string> desired_hosts;

      /**
       * A map to determine host health
       */
      std::unordered_map<std::string, HostStatus> h_status;

      /**
       * A mutex for modifying the h_status map. @TODO: it's also used
       * to read the healthy status of all nodes.  Need to have a mutex
       * per node for performance
       */
      std::mutex hs_mutex;

      /**
       * A map holding all the zombied sockets and their time of death
       */
      std::unordered_map<int32_t, uint64_t> zm;

      /**
       * A mutex for accessing the zombie sfd map
       */
      std::mutex z_mutex;
      
      /**
       * A sfd to each host:port
       */
      std::unordered_map<int32_t, std::string> conns;

      /**
       * A mutex to lock access to conns map
       */
      std::mutex conn_mutex;

      /**
       * The call backs sfd -> uuid -> callback
       */
      std::unordered_map<int32_t,  std::unordered_map<std::string, std::function<void(std::vector<char>&&)>>> call_backs;
      
      /**
       * A mutex to lock access to the call_backs map
       */
      std::mutex call_backs_mutex;

      /**
       * A map holding all the read resources SocketUtils::ReadR
       */
      std::unordered_map<int32_t, SocketUtils::ReadR> rrm;

      /**
       * A global mutex for accessing both bf_map and sfdm_map
       */
      std::mutex r_mutex;

      /**
       * A map holding all the write resources SocketUtils::WriteR
       */
      std::unordered_map<int32_t, SocketUtils::WriteR> wrm;
      
      /**
       * A mutex for handling accesses to  w_sfdm_map as well, w_cv_map and sfd_sig_sent map
       */
      std::mutex w_mutex;

      /**
       * A map of the current EPOLLIN and/or EPOLLOUT for sfd
       */
      std::unordered_map<int32_t, int32_t> sfd_events;

      /**
       * A mutex for accessing sfd_events
       */
      std::mutex e_mutex;

      /**
       * A vector for hosts that could not be reconnected to
       */
      std::vector<std::string> e_hosts;

      /**
       * A mutex for e_hosts
       */
      std::mutex eh_mutex;

      /**
       * Make a connection and store it
       */
      bool make_connection(std::string node);

      /**
       * This method is for a single thread that will continuously try to
       * connected to hosts that either failed to connect to or at some
       * point lost connection
       */
      void retry_conn();

      /**
       * This method loops for all of eternity to process e poll events
       */
      void process_epoll_events(int32_t ep_sfd, std::function<void(int32_t, int32_t)> read_callback, 
          std::function<void(int32_t, int32_t)> write_callback);

      /**
       * This method reads message off the socket file descriptor, calls the callback and removes
       * the callback from our table of callbacks
       */
      void read(int32_t ep_sfd, int32_t sfd);

      /**
       * This method writes messages on to the socket
       */
      void write(int32_t ep_sfd, int32_t sfd);

      /**
       * Method adds sfd to a set of zombied to be reaped later
       */
      void a_zombied(int32_t sfd);

      /**
       * This method starts a thread that will continuously clean up zombied socket resouces
       */
      void reap_resources();

      /**
       * Closes the server ep_sfd and sfd as well as cleans up
       */
      void close_n_clean(int32_t ep_sfd, int32_t sfd);

    public:

      /**
       * Default constructor takes a vector of host:port
       */
      SocketClient(std::vector<std::string> desired_hosts);

      /**
       * Connects to all nodes
       */
      uint32_t connect_to_hosts();

      /**
       * Sends a message on to the node that the node at the index ni.  It will not block but call the resp_callback with a response from the server.
       * If the server is down it will return false and not make the request
       */
      bool send_msg(const char *data, size_t size, uint32_t ni, std::function<void(std::vector<char>)> resp_callback, std::string &uuid_str);

      /**
       * Sends a message on to the node that the hash_key hashes to.  It will not block but call the resp_callback with a response from the server.
       * If the server is down it will return false and not make the request
       */
      bool send_msg(const char *data, size_t size, std::string &hash_key, std::function<void(std::vector<char>)> resp_callback);

      /**
       * Sends a message on to the node at index ni.  It will block and put response in the result
       */
      bool send_msg(const char *data, size_t size, uint32_t ni, std::vector<char> &result, uint64_t to_millis);

      /**
       * Sends a message on to the node that the hash_key hashes to.  It will block and put response in the result
       */
      bool send_msg(const char *data, size_t size, std::string &hash_key, std::vector<char> &result, uint64_t to_millis);

  };

}

#endif
