#include "socket_utils.hpp"

using namespace asutils;

log4cpp::Category& SocketUtils::logger = log4cpp::Category::getRoot();

/**
 * This method makes the socket non blocking
 */
void SocketUtils::unblock_socket(int32_t sfd) {

  //get the current flags
  int32_t flags = fcntl(sfd, F_GETFL, 0);
  if(flags < 0) {
    //couldn't get flags
    
    logger.error("Could not get socket flags yo!");
    exit(1);

  }

  //update the flags with non blocking flag
  flags |= O_NONBLOCK;

  int32_t set_result = fcntl(sfd, F_SETFL, flags);

  if(set_result < 0 ) {

    logger.error("Could not set socket flags yo!");
    exit(1);

  }

}

/**
 * Creates the epoll file descriptor
 */
int32_t SocketUtils::create_and_config_epoll(int32_t sfd) {

  //lets first create the epoll file descriptor
  int32_t ep_sfd = epoll_create1(0);

  if(ep_sfd < 0) {

    logger.error("Could not create epoll file descriptor yo!");
    exit(1);

  }

  //now lets configure it to listen for events
  struct epoll_event e_event;
  e_event.data.fd = sfd;
  e_event.events = EPOLLIN;

  int32_t set_result = epoll_ctl(ep_sfd, EPOLL_CTL_ADD, sfd, &e_event);

  if(set_result < 0) {

    logger.error("Could not configure epoll file descriptor yo!");
    exit(1);

  }

  return ep_sfd;

}

/**
 * This method handles adding new connections to the epoll event watcher
 */
void SocketUtils::add_fd_to_epoll(int32_t ep_sfd, int32_t sfd, std::function<void(int32_t)> sa_callback) {

  bool keep_accepting = true;

  while(keep_accepting) {

    //since we can have one or more connection requests we need
    //to accept them all 

    //This is the external client that is connecting
    struct sockaddr cin_addr;
    socklen_t cin_len;
    int cin_fd;

    //create some buffers to for the external dude
    char host_buff[NI_MAXHOST];
    char serv_buff[NI_MAXSERV];

    cin_len = sizeof(cin_addr);

    //accept the connection and give the accept some buffers to fill up
    cin_fd = accept(sfd, &cin_addr, &cin_len);

    if(cin_fd < 0) {

      if((errno == EAGAIN) ||
          (errno == EWOULDBLOCK)) {

        //yay we have handled all new connection requests
        //so just break the loop and be done with it you!
        keep_accepting = false;

      } else {

        //on noes.  there was an error lets say something and still break
        logger.error(std::string("There was an error accepting a new connection: ") + std::to_string(errno));
        keep_accepting = false;

      }

    } else {
      
      //we get in this block if we accepted successfully
      int32_t get_name_result = getnameinfo( &cin_addr, cin_len, host_buff, sizeof(host_buff), serv_buff,
          sizeof(serv_buff), NI_NUMERICHOST | NI_NUMERICSERV);

      if(get_name_result == 0) {

        logger.info(std::string("Incoming connection from fd: ") + std::to_string(cin_fd) + std::string(" host: ") + std::string(host_buff) + std::string(" port: ") + std::string(serv_buff)); 

        //got a valid incoming connection, lets unblock it and add it to the list of homies to watch
        //unblock the new fd
        unblock_socket(cin_fd);

        //add the new fd to epoll to watch events on
        struct epoll_event e_event;
        e_event.data.fd = cin_fd;
        e_event.events = EPOLLIN;

        uint32_t add_result = epoll_ctl(ep_sfd, EPOLL_CTL_ADD, cin_fd, &e_event);

        if(add_result < 0) {

          logger.error("Couldn't add the new fd to the epoll pool yo!");

        } else {

          //callback on each successfull add
          sa_callback(cin_fd);
          
        }

      } else {

        logger.error("Could not get the name of the incoming client yo!");

      }

    }

  }

}

/**
 * This method drains the sfd into a buffered reader until it is told to stop by epoll
 */
void SocketUtils::read_from_sfd(int32_t ep_sfd, int32_t sfd, BufferedReader &reader, std::function<void()> close_callback, 
    std::unordered_map<int32_t, int32_t> &sfd_events, std::mutex &e_mutex) {

  //a flag to determine if we are done reading
  bool keep_reading = true;

  bool set_epollin = true;

  while(keep_reading) {

    char r_buff[1024];

    //lets read some bytes into a buffer
    ssize_t bytes_read = read(sfd, r_buff, sizeof(r_buff));

    if( bytes_read < 0 ) {

      if(errno == EAGAIN) {

        logger.error(std::string("Could not read from the socket ") + std::to_string(sfd) + std::string(" Waiting for EPOLLIN: ") + std::to_string(errno));
        
      } else {
        
        logger.error(std::string("Error reading: ") + std::to_string(errno));
        close_callback();
        set_epollin = false;
        keep_reading = false; 

      }

      //if less than 0 then we have an error
      keep_reading = false;


    } else if(bytes_read == 0) {
      
      //we get in this block if the remote client closes the conn
      logger.info("Remote has closed connection");
      close_callback();

      set_epollin = false;
      keep_reading = false; 
      
    } else {

      reader.read(r_buff, bytes_read);

    }

  }

  if(set_epollin) {

    e_mutex.lock();
    sfd_events[sfd] |= EPOLLIN;
    SocketUtils::set_epoll(ep_sfd, sfd, sfd_events[sfd] );
    e_mutex.unlock();

  }

}

/**
 * This tells epoll that we need to know when you write so that we can write too
 */
void SocketUtils::set_epoll(int32_t ep_sfd, int32_t sfd, int32_t events) {

  struct epoll_event e_event;
  e_event.data.fd = sfd;
  e_event.events = events;

  uint32_t update_result = epoll_ctl(ep_sfd, EPOLL_CTL_MOD, sfd, &e_event);
  if(update_result < 0) {

    logger.error("Couldn't update the fd to signal!");

  }

}

/**
 * This method drains the sockets write buffer until it is empty or
 * it's told not to by epoll
 */
void SocketUtils::write_to_sfd(int32_t ep_sfd, int32_t sfd, BufferedWriter &writer, std::function<void()> close_callback,
    std::unordered_map<int32_t, int32_t> &sfd_events, std::mutex &e_mutex) {

  bool keep_writing = true;

  while(keep_writing && writer.size() > 0) {

    //let's get some bytes out of our writer
    size_t b_size = 1024;

    if(writer.size() < b_size) {
      //if what we have to write is less than 1024 
      //let's set our size to what's in the buffer

      b_size = writer.size();
    
    }

    char r_buff[b_size];
    writer.iread(r_buff, b_size);

    //attempt to send it off
    int32_t r = write(sfd, r_buff, b_size);

    if( r < 0) {

      if(errno == EAGAIN) {

        logger.error(std::string("Could not write on the socket ") + std::to_string(sfd) + std::string(" Waiting for EPOLLOUT: ") + std::to_string(errno));
        e_mutex.lock();
        sfd_events[sfd] |= EPOLLOUT;
        SocketUtils::set_epoll(ep_sfd, sfd, sfd_events[sfd] );
        e_mutex.unlock();

      } else {

        logger.error(std::string("Could not write on the socket.  Cannot continue: ") + std::to_string(errno));
        //close this sfd and return false
        close_callback();

      }

      keep_writing = false;
      
    } else {

      //if we wrote successfully remote the written bytes from our buffer
      writer.clear(b_size);

    }

  }

}

/**
 * Creates a message frame
*/
void SocketUtils::pack_frame(const char *id, const char* msg, size_t msg_size,  char *result) {

  //the index of the final frame
  uint32_t r_i = 0;

  //first lets add the uuid
  for(uint8_t i=0; i < 37; ++i) {

    result[r_i++] = id[i];

  }

  //next lets add the message
  for(uint32_t i=0; i < msg_size; ++i) {

    result[r_i++] = msg[i];

  }

  //finally lets add the end of transmission del
  char del = (char) 4;
  result[r_i] = del;

}

/**
 * Creates a message frame
*/
void SocketUtils::pack_frame(std::vector<char> &id, std::vector<char> &msg, char *result) {

  uint32_t r_i = 0;

  //first lets add the uuid
  for(uint8_t i=0; i < 37; ++i) {

    result[r_i++] = id[i];

  }

  //next lets add the message
  for(uint32_t i=0; i < msg.size(); ++i) {

    result[r_i++] = msg[i];

  }

  //finally lets add the end of transmission del
  char del = (char) 4;
  result[r_i] = del;

}

/**
 * Unpacks a message frame
*/
void SocketUtils::unpack_frame(const char *msg, size_t msg_size, std::vector<char> &uuid_v, std::vector<char> &msg_v) {

  //read until the second to last char as that's the delimiter
  for(uint32_t i=0; i < (msg_size-1); ++i) {

    if(i < 37) {

      //bytes 0-15 are the uuid
      uuid_v.push_back(msg[i]);

    } else {

      //all other bytes are the msg
      msg_v.push_back(msg[i]);
    }

  }

}
