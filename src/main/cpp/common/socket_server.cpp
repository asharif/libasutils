#include "socket_server.hpp"

using namespace asutils;

log4cpp::Category& SocketServer::logger = log4cpp::Category::getRoot();

/**
 * Default constructor takes a port to listen to
 */
SocketServer::SocketServer(const uint32_t port, std::function<void(std::vector<char> &&uuid_v, std::vector<char> &&msg_v, 
            SocketServer &server, const int32_t sfd)> handler) : a_tp(std::thread::hardware_concurrency()), 
  r_tp(std::thread::hardware_concurrency()), w_tp(std::thread::hardware_concurrency()) {

  //ignore sigpipe
  std::signal(SIGPIPE, SIG_IGN);

  this->handler = handler;
  this->port = port;

  create_socket();
  bind_socket();

  //make socket non blocking
  SocketUtils::unblock_socket(this->i_sfd);

  //listen on the socket
  listen_socket();

  //configure epoll
  this->ep_sfd = SocketUtils::create_and_config_epoll(this->i_sfd);

  //create some epoll callbacks
  
  //this one is for adding connections
  std::function<void()> add_callback = [this]() { 

    //add to our add conn threadpool
    std::function<void()> add_f = [this]()  { 

      std::function<void(int32_t)> sa_callback = [this](int32_t nsfd) {

        this->add(nsfd); 

      };
      
      SocketUtils::add_fd_to_epoll(this->ep_sfd, this->i_sfd, sa_callback);
    
    };

    a_tp.add_work(add_f);

  };

  //this one is for reading data
  std::function<void(int32_t)> read_callback = [this](int32_t sfd) {

    //turn off EPOLLIN notifications for this sfd
    this->e_mutex.lock();
    this->sfd_events[sfd] ^= EPOLLIN;
    SocketUtils::set_epoll(this->ep_sfd, sfd, this->sfd_events[sfd]);
    this->e_mutex.unlock();
    
    //add the read to our read threadpool
    std::function<void()> read_f = [this, sfd]() { this->read(sfd);  };
    r_tp.add_work(read_f);
  
  };

  //this one is for writing data
  std::function<void(int32_t)> write_callback = [this](int32_t sfd) {

    //turn off EPOLLOUT notifications for this sfd
    this->e_mutex.lock();
    this->sfd_events[sfd] ^= EPOLLOUT;
    SocketUtils::set_epoll(this->ep_sfd, sfd, this->sfd_events[sfd]);
    this->e_mutex.unlock();

    //add the write to our write threadpool
    std::function<void()> write_f = [this, sfd]() { this->write(sfd); };
    w_tp.add_work(write_f);

  };

  //start zombied resource reaper
  std::thread pt(&SocketServer::reap_resources, this);
  pt.detach();
  
  process_epoll_events(add_callback, read_callback, write_callback);
  
}

/**
 * This method creates a socket to listen on and returns it.  If it fails
 * the process will exit
 */
void SocketServer::create_socket(){

  //create a socket of time internet (instead of unix and of type TCP instead of UDP)
  this->i_sfd = socket(AF_INET, SOCK_STREAM, 0);

  if(this->i_sfd < 0) {

    //for some reason we could create the socket file descriptor
    logger.error("Could not create the socket yo!");
    exit(1);

  }

  //we need to bzero the bytes to not have any hot garbage in there on accident
  bzero((char *) &this->serv_addr, sizeof(this->serv_addr));
  //now lets set some settings on the sockaddr
  //we are going over the internet with this socket not in the unix domain
  this->serv_addr.sin_family = AF_INET;
  //lets bind to any network interface that is connected to the internet
  this->serv_addr.sin_addr.s_addr = INADDR_ANY;
  //lets convert the port number to network byte order or big endian
  this->serv_addr.sin_port = htons(this->port);

}

void SocketServer::bind_socket(){

  //lets attempt to bind the socket to the sockaddr wich contains information like 
  //the type of socket and the port
  int32_t b_result = bind(this->i_sfd, (struct sockaddr *) &this->serv_addr, sizeof(this->serv_addr));

  if(b_result < 0) {

    //for some reason we couldn't bind to the port
    logger.error(std::string("Could not bind to the port: ") + std::to_string(this->serv_addr.sin_port));
    exit(1);

  }

}

/**
 * This method adds new incomming connections
 */
void SocketServer::add(int32_t nsfd) {

  //the callback for once we have a full message frame
  std::function<void(std::vector<char>)> call_back = [this, nsfd](std::vector<char> msg_frame) {

    //we need to add the del since the buffered reader strips it
    msg_frame.push_back((char) 4);

    //the uuid and the message placeholders
    std::vector<char> uuid_v;
    std::vector<char> msg_v;

    //unpack the message
    SocketUtils::unpack_frame(&msg_frame[0], msg_frame.size(), uuid_v, msg_v);
    this->handler(std::move(uuid_v), std::move(msg_v), *this, nsfd);

  };

  //init the read resources
  this->r_mutex.lock();
  struct SocketUtils::ReadR nrr;
  nrr.br = BufferedReader((char) 4, call_back);
  nrr.mutex = new std::mutex();
  nrr.is_valid = true;
  //if we don't have an entry lets create one
  this->rrm.emplace(nsfd, nrr);
  this->r_mutex.unlock();

  //init the write resources
  this->w_mutex.lock();
  struct SocketUtils::WriteR nwr;
  nwr.mutex = new std::mutex();
  nwr.is_valid = true;
  //if we don't have an entry lets create one
  this->wrm.emplace(nsfd, nwr);
  this->w_mutex.unlock();

  //init the epoll resources
  this->e_mutex.lock();
  this->sfd_events[nsfd] = EPOLLIN;
  this->e_mutex.unlock();

}

/**
 * This method loops for all of eternity to process e poll events
 */
void SocketServer::process_epoll_events(std::function<void()> add_callback, std::function<void(int32_t)> read_callback, 
    std::function<void(int32_t)> write_callback) {

  struct epoll_event *e_events;

  //lets arbitrarily watch for a maximum of 64 events.  have no idea why but the example I saw
  //uses 64
  uint8_t max_events = 64;
  e_events = (epoll_event*)calloc (max_events, sizeof(epoll_event));

  while(1) {
    
    //lets wait for events here for ever
    int32_t n_events = epoll_wait(this->ep_sfd, e_events, max_events, -1);

    if(n_events < 0 ) {

      logger.error(std::string("Error happened waiting for epoll events (errno): ") + std::to_string(errno));
    } 

    //yay we got events yo!
    for(uint8_t i =0; i <  n_events; ++i) {

      if((e_events[i].events & EPOLLERR) ||
          (e_events[i].events & EPOLLHUP)) {

        //EPOLLERR an arror happened
        //EPOLLHUP the file descriptor hung up
        //We shouldn't get in here

        logger.error("We got some type of epoll error, closing the file descriptor and moving on yo! ");
        //close the associated file descripter 
        
        a_zombied(e_events[i].data.fd);
        close(e_events[i].data.fd);
        
      } else if (e_events[i].data.fd == this->i_sfd) {

        //we got a connection request yo!
        //let's call the add callback provided
        add_callback();

      } else {
        
        if(e_events[i].events & EPOLLIN) {

          //if we are in this block then we have data to read yo!
          //let's call the read_callback provided
          read_callback(e_events[i].data.fd);

        } 
        
        if(e_events[i].events & EPOLLOUT) {

          //if we are in this block then we are able to write
          //call the callback
          write_callback(e_events[i].data.fd);
          
        } 

      }

    }

  }

}

/**
 * Start listening on the socket
 */
void SocketServer::listen_socket() {

  //listen on the socket yo
  int32_t listen_result = listen(this->i_sfd, SOMAXCONN);

  if(listen_result < 0) {

    logger.error("Could not listen on socket yo!");
    exit(1);

  }

}

/**
 * This method reads message off the socket file descriptor, calls the callback 
 */
void SocketServer::read(int32_t sfd) {

  //grab a lock
  this->r_mutex.lock();

  //get our our resource
  SocketUtils::ReadR *rr = NULL;
  
  //lets first get our buffered reader out for this sfd
  std::unordered_map<int32_t, SocketUtils::ReadR>::const_iterator rr_got = this->rrm.find(sfd);
	if( rr_got != this->rrm.end()) {

    rr = &this->rrm.find(sfd)->second;
    
	}

  this->r_mutex.unlock();

  if(rr != NULL) {

    //create a callback for a client close scenario
    std::function<void()> close_callback = [&sfd, this, &rr]() {

      //unset valid
      rr->is_valid = false;
      this->a_zombied(sfd);
      
    };
    
    //now lets finally lock on this sfd mutex
    rr->mutex->lock();

    if(rr->is_valid) {

      //only do stuff if we haven't been marked for death
      SocketUtils::read_from_sfd(this->ep_sfd, sfd, rr->br, close_callback, 
          this->sfd_events, this->e_mutex);

    }

    //unlock after read
    rr->mutex->unlock();

  }

}

/**
 * This method writes messages on to the socket
 */
void SocketServer:: write(int32_t sfd) {

  //grab a global write lock
  this->w_mutex.lock();

  //get the resource
  SocketUtils::WriteR *wr = NULL;

  //let's get our buffered writer out for this sfd
  std::unordered_map<int32_t, SocketUtils::WriteR>::const_iterator wr_got = this->wrm.find(sfd);
  if( wr_got != this->wrm.end()) {

    wr = &this->wrm.find(sfd)->second;
    
  }

  //release the global write lock
  this->w_mutex.unlock();

  if(wr != NULL) {

    //create a callback for a client close scenario
    std::function<void()> close_callback = [&sfd, this, &wr]() {
      
      //unset valid
      wr->is_valid = false;
      this->a_zombied(sfd);
      
    };

    //grab the sfd write lock
    wr->mutex->lock();

    if(wr->is_valid) {

      //only do stuff if we haven't been marked for death
      SocketUtils::write_to_sfd(this->ep_sfd, sfd, wr->bw, close_callback,
          this->sfd_events, this->e_mutex);
    }

    //release the sfd write lock
    wr->mutex->unlock();

  }

}

/**
 * Add message to a BufferedWriter for this sfd
 */
void SocketServer::send_msg(std::vector<char> &uuid_v, std::vector<char> &msg_v, const int32_t sfd) {

  //make a buffer just big enough for uuid + size + del
  uint32_t mfs = 37+msg_v.size()+1;
  char msg_frame[mfs]; 
  //pack it neatly into a frame
  SocketUtils::pack_frame(uuid_v, msg_v, msg_frame);

  //grab a mutex for read
  this->w_mutex.lock();

  //get the resource
  SocketUtils::WriteR *wr = NULL;

  //let's get our buffered writer out for this sfd
  std::unordered_map<int32_t, SocketUtils::WriteR>::const_iterator wr_got = this->wrm.find(sfd);
  if( wr_got != this->wrm.end()) {

    wr = &this->wrm.find(sfd)->second;
    
  }

  //release the read lock
  this->w_mutex.unlock();

  if(wr != NULL) {

    //grab the sfd write lock
    wr->mutex->lock();

    if(wr->is_valid) {

      //if this resource is not dead then write to the buffered writer 
      wr->bw.write(msg_frame, mfs);

    }

    //release the sfd write lock
    wr->mutex->unlock();

    //tell empoll to let us know when we can write cause we have stuff to write
    //grab a shared read lock
    this->e_mutex.lock();
    sfd_events[sfd] |= EPOLLOUT;
    SocketUtils::set_epoll(this->ep_sfd, sfd, sfd_events[sfd] );
    this->e_mutex.unlock();

  }

}

/**
 * Closes the client sfd as well as cleans up
 */
void SocketServer::close_n_clean(int32_t sfd) {
  
  logger.info(std::string("Cleaning up resources for sfd: ") + std::to_string(sfd));
  
  //close the socket
  close(sfd);

  //clean up read resources
  this->r_mutex.lock();
  std::unordered_map<int32_t, SocketUtils::ReadR>::const_iterator rr_got = this->rrm.find(sfd);
  if( rr_got != this->rrm.end()) {

    delete this->rrm[sfd].mutex;
    this->rrm.erase(sfd);

  }
  this->r_mutex.unlock();

  //clean up wrie resources
  this->w_mutex.lock();
  std::unordered_map<int32_t, SocketUtils::WriteR>::const_iterator wr_got = this->wrm.find(sfd);
  if( wr_got != this->wrm.end()) {

    delete this->wrm[sfd].mutex;
    this->wrm.erase(sfd);
  }
  this->w_mutex.unlock();

  //clean up epoll event resources
  this->e_mutex.lock();
  this->sfd_events.erase(sfd);
  this->e_mutex.unlock();

  logger.info(std::string("Finished cleaning up resources for sfd: ") + std::to_string(sfd));

}

/**
 * Method adds sfd to a set of zombied to be reaped later
 */
void SocketServer::a_zombied(int32_t sfd) {

  //grab lock
  this->z_mutex.lock();
  //add this sfd to zombied set
  this->zm[sfd] = Utils::epoch_millis_now();
  //release the zombie set lock
  this->z_mutex.unlock();

}

/**
 * This method starts a thread that will continuously clean up zombied socket resouces
 */
void SocketServer::reap_resources() {

  logger.info("Starting zombied resource reaper");

  while(1) {

    //sleep for 30 seconds and then loop
    std::this_thread::sleep_for(std::chrono::seconds(30));
    logger.info("Reaping zombied resources");

    //grab the zombied sfd lock
    this->z_mutex.lock();

    std::vector<int32_t> zv;
    for(auto it = this->zm.begin(); it != this->zm.end(); ++it ) {

      //reap after 60 seconds of being marked for death
     if((Utils::epoch_millis_now() - it->second) > 60000) {

        //close and clean resources
        this->close_n_clean(it->first);
        zv.emplace_back(it->first);

      }

    }

    for(int32_t &zsfd : zv) {
      
      //now remove the entries from our zombie set
      this->zm.erase(zsfd);
    
    }

    this->z_mutex.unlock();


  }

}


