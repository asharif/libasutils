#ifndef AS_UTILS_SOCKET_SERVER_HPP
#define AS_UTILS_SOCKET_SERVER_HPP

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include "socket_utils.hpp" 
#include "utils.hpp" 
#include "thread_pool.hpp"
#include "buffered_reader.hpp"
#include "buffered_writer.hpp"
#include <unordered_map>
#include <csignal>
#include <chrono>
#include <mutex>
#include <log4cpp/Category.hh>

namespace asutils {


  class SocketServer {
		
    private:

      static log4cpp::Category &logger;

      /**
       * A struct that holds information on the type of socket
       */
      struct sockaddr_in serv_addr;

      /**
       * The epoll event struct that we are going to listen on.
       * This contains the listener socket fd as well as all fds once
       * a connection has been established
       */
      struct epoll_event e_event;

      /**
       * Threadpool for adding connections
       */
      ThreadPool a_tp;

      /**
       * Threadpool for reading data
       */
      ThreadPool r_tp;

      /**
       * Threadpool for writing data
       */
      ThreadPool w_tp;

      /**
       * A map holding all the SocketUtils::ReadR for a sfd 
       */
      std::unordered_map<int32_t, SocketUtils::ReadR> rrm;

      /**
       * A global mutex for accessing the rr map 
       */
      std::mutex r_mutex;

      /**
       * A map holding all the SocketUtils::ReadR for a sfd 
       */
      std::unordered_map<int32_t, SocketUtils::WriteR> wrm;

      /**
       * A global mutex for accessing the rr map 
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
       * A map holding all the zombied sockets and their time of death
       */
      std::unordered_map<int32_t, uint64_t> zm;

      /**
       * A mutex for accessing the zombie sfd map
       */
      std::mutex z_mutex;

      /**
       * A function to handle messages
       */
      std::function<void(std::vector<char> &&uuid_v, std::vector<char> &&msg_v, 
            SocketServer &server, const int32_t sfd)> handler;

      /**
       * This is the initial socket file descriptor
       */
      int32_t i_sfd;

      /**
       * This is the port we are going to listen on
       */
      uint32_t port;

      /**
       * This is the epoll file describtor
       */
      int32_t ep_sfd;
      
      /**
       * This method creates a socket to listen on and returns it.  If it fails
       * the process will exit
       */
      void create_socket();
      
      /**
       * This method attempt to bind with the address. If it fails the process will
       * exit
       */
      void bind_socket();

      /**
       * This method adds new incomming connections
       */
      void add(int32_t sfd);

      /**
       * This method loops for all of eternity to process e poll events
       */
      void process_epoll_events(std::function<void()> add_callback, std::function<void(int32_t)> read_callback, 
          std::function<void(int32_t)> write_callback);

      /**
       * Start listening on the socket
       */
      void listen_socket();

      /**
       * This method reads message off the socket file descriptor
       */
      void read(int32_t sfd);

      /**
       * This method writes messages on to the socket
       */
      void write(int32_t sfd);

      /**
       * Closes the client sfd as well as cleans up
       */
      void close_n_clean(int32_t sfd);

      /**
       * Method adds sfd to a set of zombied to be reaped later
       */
      void a_zombied(int32_t sfd);

      /**
       * This method starts a thread that will continuously clean up zombied socket resouces
       */
      void reap_resources();

    public:

      /**
       * Default constructor takes a port to listen to
       */
      SocketServer(const uint32_t port, std::function<void(std::vector<char> &&uuid_v, std::vector<char> &&msg_v, 
            SocketServer &server, const int32_t sfd)> handler); 

      /**
       * Send message on socket file descriptor
       */
      void send_msg(std::vector<char> &uuid_v, std::vector<char> &msg_v, const int32_t sfd);

  };


}

#endif
