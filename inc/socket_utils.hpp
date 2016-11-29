#ifndef AS_UTILS_EPOLL_UTILS_HPP
#define AS_UTILS_EPOLL_UTILS_HPP

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
#include <functional>
#include "buffered_reader.hpp"
#include "buffered_writer.hpp" 
#include <uuid/uuid.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "utils.hpp" 
#include <log4cpp/Category.hh>

namespace asutils {

  class SocketUtils {

    private:

      static log4cpp::Category &logger;

    public:

      /**
       * These are the read resources
       */
      struct ReadR {

        BufferedReader br;
        std::mutex *mutex;
        bool is_valid; 

      };

      /**
       * These are the read resources
       */
      struct WriteR {

        BufferedWriter bw;
        std::mutex *mutex;
        bool is_valid; 

      };

      /**
       * This method makes the socket non blocking
       */
      static void unblock_socket(int32_t sfd);

      /**
       * Creates the epoll file descriptor
       */
      static int32_t create_and_config_epoll(int32_t sfd);

      /**
       * This method handles adding new connections to the epoll event watcher
       */
      static void add_fd_to_epoll(int32_t ep_sfd, int32_t sfd, 
          std::function<void(int32_t)> sa_callback);

      /**
       * This updates epoll events
       */
      static void set_epoll(int32_t ep_sfd, int32_t sfd, int32_t events);

      /**
       * This method drains the sfd into a buffered reader until it is told to stop by epoll
       */
      static void read_from_sfd(int32_t ep_sfd, int32_t sfd, BufferedReader &reader, std::function<void()> close_callback, 
          std::unordered_map<int32_t, int32_t> &sfd_events, std::mutex &e_mutex);

      /**
       * This method drains the sockets write buffer until it is empty or
       * it's told not to by epoll
       */
      static void write_to_sfd(int32_t ep_sfd, int32_t sfd, BufferedWriter &writer, std::function<void()> close_callback,
          std::unordered_map<int32_t, int32_t> &sfd_events, std::mutex &e_mutex);

      /**
       * Creates a message frame
       */
      static void pack_frame(const char *id, const char* msg, size_t msg_size, char *result);

      /**
       * Creates a message frame
       */
      static void pack_frame(std::vector<char> &id, std::vector<char> &msg, char *result);

      /**
       * Unpacks the message frame
       */
      static void unpack_frame(const char *msg, size_t msg_size, std::vector<char> &uuid_v, std::vector<char> &msg_v);

  };

}

#endif
