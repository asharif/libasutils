#include "socket_client.hpp"

using namespace asutils;

log4cpp::Category& SocketClient::logger = log4cpp::Category::getRoot();

/**
 * Default constructor takes a vector of host:port
 */
SocketClient::SocketClient(std::vector<std::string> desired_hosts) : r_tp(std::thread::hardware_concurrency()),
  w_tp(std::thread::hardware_concurrency()) {

    //ignore sigpipe
    std::signal(SIGPIPE, SIG_IGN);

    this->desired_hosts = desired_hosts;

    //start the zombied reaper
    std::thread z_thread (&SocketClient::reap_resources, this);
    z_thread.detach();

    //start a thread that will attempt to reconnect to hosts
    std::thread r_thread (&SocketClient::retry_conn, this);
    r_thread.detach();

  }

/**
 * Connects to all nodes
 */
uint32_t SocketClient::connect_to_hosts() {

  //keep a counter to the number of hosts that were successfully connected to
  //it's the callers responsibility to determine what to do with that info
  uint32_t s_conns = 0;
  //make a connection to all the hosts
  for(std::string &t : this->desired_hosts) {

    if(make_connection(t)) {

      s_conns++;

    } else {

      //add this host to the failed list to reattempt later
      this->eh_mutex.lock();
      this->e_hosts.push_back(t);
      this->eh_mutex.unlock();
    }

  }

  return s_conns;

}

/**
 * Make a connection and store it
 */
bool SocketClient::make_connection(std::string node) {

  logger.info(std::string("Connecting to remote host: ") + std::string(node)) ;

  //flag to determine if connected to at least one healthy host
  bool success = false;

  //init the host status struct
  struct HostStatus status;
  status.is_healthy = false;

  std::vector<std::string> hp = Utils::split(node, ':');
  std::string host;
  uint32_t port;

  try {

    host = hp[0];
    port  = std::stoi(hp[1]);
    success = true;

  } catch (const std::exception &e) {

    logger.error(std::string("Could not parse host: ") + std::string(node) + std::string(" ") + std::string(e.what()));

  }

  if(success) {

    //we get into this block of we parsed correctly

    struct hostent *server = gethostbyname(host.c_str());
    int32_t sfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sfd < 0) {

      logger.error("Could not create socket");
      success = false;

    } else {

      struct sockaddr_in serv_addr;

      //zero out the bytes
      bzero((char *) &serv_addr, sizeof(serv_addr));

      //make the address an internet one
      serv_addr.sin_family = AF_INET;

      //copy some header information into the server
      bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);

      //change to big endian
      serv_addr.sin_port = htons(port);

      //attempt to connect to the server
      int32_t c_result = connect(sfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

      if(c_result < 0) {

        logger.error(std::string("Could not connect to remote host: ") + std::string(node));
        success = false;

      } else {

        //we connected successfully here
        //make socket non blocking
        SocketUtils::unblock_socket(sfd);

        //configure epoll
        int ep_sfd = SocketUtils::create_and_config_epoll(sfd);

        //lets set host status info as now we connected
        status.is_healthy = true;
        status.sfd = sfd;
        status.ep_sfd = ep_sfd;

        //grab a lock to modify the conns map
        this->conn_mutex.lock();
        //let's now add it to our map
        this->conns[sfd] = node; 
        //release the lock
        this->conn_mutex.unlock();

        //init the new connection to give us EPOLLIN events
        this->e_mutex.lock();
        this->sfd_events[sfd] = EPOLLIN;
        this->e_mutex.unlock();

        //create some epoll callback

        //this one is for reading data
        std::function<void(int32_t, int32_t)> read_callback = [this](int32_t ep_sfd, int32_t sfd) {

          //turn off EPOLLIN notifications for this sfd
          this->e_mutex.lock();
          this->sfd_events[sfd] ^= EPOLLIN;
          SocketUtils::set_epoll(ep_sfd, sfd, this->sfd_events[sfd]);
          this->e_mutex.unlock();

          //add the read to our processing threadpool
          std::function<void()> process_f = [this, ep_sfd, sfd]() { this->read(ep_sfd, sfd); };
          r_tp.add_work(process_f);

        };

        //this one is for writing data
        std::function<void(int32_t, int32_t)> write_callback = [this](int32_t ep_sfd, int32_t sfd) {

          //turn off EPOLLOUT notifications for this sfd
          this->e_mutex.lock();
          this->sfd_events[sfd] ^= EPOLLOUT;
          SocketUtils::set_epoll(ep_sfd, sfd, this->sfd_events[sfd]);
          this->e_mutex.unlock();

          //add the write to our write threadpool
          std::function<void()> write_f = [this, ep_sfd, sfd]() { this->write(ep_sfd, sfd); };
          w_tp.add_work(write_f);

        };

        std::thread pt(&SocketClient::process_epoll_events, this, ep_sfd, read_callback, write_callback);
        pt.detach();

      }

    }

  }

  this->hs_mutex.lock();
  //make the host status entry
  this->h_status[node] = status;
  this->hs_mutex.unlock();

  return success;

}

/**
 * This method continuously tries to reconnect to nodes
 * that were could not be connected to 
 */
void SocketClient::retry_conn() {

  while(1) {

    //sleep for a bit and try again
    std::this_thread::sleep_for(std::chrono::seconds(10));

    this->eh_mutex.lock();

    //a vector to hold the indices of the successfully reconnected nodes
    std::vector<uint32_t> sci;
    for(uint32_t i=0; i < this->e_hosts.size(); ++i) {

      logger.info(std::string("Attempting to reconnect to host: ") + std::string(this->e_hosts[i]));
      if(make_connection(this->e_hosts[i])) {

        //if connection was successfull we need to remove it 
        //from this list
        sci.push_back(i);

      }

    }

    for(uint32_t i=0; i < sci.size(); ++i) {

      //remove the successfully reconnected hosts
      this->e_hosts.erase(this->e_hosts.begin() + sci[i]);

    }

    this->eh_mutex.unlock();

  }

}

/**
 * This method loops for all of eternity to process e poll events
 */
void SocketClient::process_epoll_events(int32_t ep_sfd, std::function<void(int32_t, int32_t)> read_callback,
    std::function<void(int32_t, int32_t)> write_callback) {

  struct epoll_event *e_events;

  //lets arbitrarily watch for a maximum of 64 events.  have no idea why but the example I saw
  //uses 64
  uint8_t max_events = 64;
  e_events = (epoll_event*)calloc (max_events, sizeof(epoll_event));

  while(1) {

    //lets wait for events here for ever
    int32_t n_events = epoll_wait(ep_sfd, e_events, max_events, -1);

    if(n_events < 0 ) {

      logger.error(std::string("Error happened waiting for epoll events (errno): ") + std::to_string(errno));
      //we need to get out of this loop and reconnect at some point
      close(ep_sfd);
      return;

    }  

    //yay we got events yo!
    for(uint8_t i =0; i <  n_events; ++i) {

      if((e_events[i].events & EPOLLERR) ||
          (e_events[i].events & EPOLLHUP)) { 

        //EPOLLERR an arror happened
        //EPOLLHUP the file descriptor hung up
        //We shouldn't get in here

        logger.error("We got some type of epoll error, closing the file descriptor and continuting yo!");
        //close the associated file descripter 
        a_zombied(e_events[i].data.fd);
        close(ep_sfd);
        close(e_events[i].data.fd);


      } else  {

        if(e_events[i].events & EPOLLIN) {

          //if we are in this block then we have data to read yo!
          //let's call the read_callback provided
          read_callback(ep_sfd, e_events[i].data.fd);

        } 

        if(e_events[i].events & EPOLLOUT) {

          //if we are in this block then we are able to write
          //call the callback
          write_callback(ep_sfd, e_events[i].data.fd);

        } 

      } 

    }

  }

}

/**
 * This method reads message off the socket file descriptor, calls the callback and removes
 * the callback from our table of callbacks
 */
void SocketClient::read(int32_t ep_sfd, int32_t sfd) {

  //the callback for once we have a full message frame
  std::function<void(std::vector<char>)> call_back = [sfd, ep_sfd, this](std::vector<char> msg_frame) {

    //we need to add the del since the buffered reader strips it
    msg_frame.push_back((char) 4);

    //the uuid and the message placeholders
    std::vector<char> uuid_v;
    std::vector<char> msg_v;

    //unpack the message
    SocketUtils::unpack_frame(&msg_frame[0], msg_frame.size(), uuid_v, msg_v);

    //convert the uuid_v to a string
    std::string uuid_str(uuid_v.begin(), uuid_v.end());

    //grab a lock and get 'da callback
    this->call_backs_mutex.lock();

    std::function<void(std::vector<char>)> da_callback = this->call_backs[sfd][uuid_str];
    //remove it from our callback map

    std::string msg_str(msg_v.begin(), msg_v.end());
    this->call_backs[sfd].erase(uuid_str);

    //release the lock
    this->call_backs_mutex.unlock();

    try {

      //call the callback
      da_callback(std::move(msg_v)); 

    } catch(std::exception &e) {

      //no callback found! something is wrong with this sfd let's add it to zombied
      this->a_zombied(sfd);
      logger.error("Could not locate callback!  This is very very bad!");
    }

  };

  //grab a lock and check for a buffered reader exists
  this->r_mutex.lock();

  //lets first get our buffered reader out for this sfd
  std::unordered_map<int32_t, SocketUtils::ReadR>::const_iterator rr_got = this->rrm.find(sfd);
  if( rr_got == this->rrm.end()) {

    //if we don't have an entry lets create one
    struct SocketUtils::ReadR nrr;
    nrr.br = BufferedReader((char) 4, call_back);
    nrr.mutex = new std::mutex();
    nrr.is_valid = true;
    this->rrm.emplace(sfd, std::move(nrr));

  }

  SocketUtils::ReadR *rr = &this->rrm.find(sfd)->second;

  //release the lock as now we have the shared resources we need
  this->r_mutex.unlock();

  //create a callback for a client close scenario
  std::function<void()> close_callback = [&sfd, this, &rr]() {

    //unset valid
    rr->is_valid = false;
    this->a_zombied(sfd);

  };

  //now lets finally lock on this sfd mutex
  rr->mutex->lock();

  if(rr->is_valid) {

    SocketUtils::read_from_sfd(ep_sfd, sfd, rr->br, close_callback, 
        this->sfd_events, this->e_mutex);

  }

  //unlock after read
  rr->mutex->unlock();

}

/**
 * This method writes messages on to the socket
 */
void SocketClient::write(int32_t ep_sfd, int32_t sfd) {

  //grab a global write lock
  this->w_mutex.lock();

  //let's get our buffered writer out for this sfd
  std::unordered_map<int32_t, SocketUtils::WriteR>::const_iterator wr_got = this->wrm.find(sfd);
  if( wr_got == this->wrm.end()) {

    //if we don't have an entry lets create one
    struct SocketUtils::WriteR nwr;
    nwr.mutex = new std::mutex();
    nwr.is_valid = true;
    this->wrm.emplace(sfd, std::move(nwr));

  }

  SocketUtils::WriteR *wr = &this->wrm.find(sfd)->second;

  //release the global write lock
  this->w_mutex.unlock();

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
    SocketUtils::write_to_sfd(ep_sfd, sfd, wr->bw, close_callback,
        this->sfd_events, this->e_mutex);
  }

  //release the sfd write lock
  wr->mutex->unlock();

}

/**
 * Sends a message on to the node that the hash_key hashes to
 */
bool SocketClient::send_msg(const char *data, size_t size, std::string &hash_key, std::function<void(std::vector<char>)> resp_callback) {

  bool result = true;

  //lets get the sfd that this message will be sent to
  size_t hash =  Utils::hash_it(hash_key);
  uint32_t ni;
  int32_t sfd = -1;
  int32_t ep_sfd = -1;

  ni = hash % this->desired_hosts.size();

  //lets check to see if host is healthy
  this->hs_mutex.lock();
  result = this->h_status[this->desired_hosts[ni]].is_healthy;
  this->hs_mutex.unlock();

  if(result) {

    //the host is healthy so we get the sfd and ep_sfd
    sfd = this->h_status[this->desired_hosts[ni]].sfd;
    ep_sfd = this->h_status[this->desired_hosts[ni]].ep_sfd;

    //grab the uuid first to represent this request
    std::string uuid_str = Utils::build_uuid_str();

    //now let's register the callback.  We will have to deregister it if we can't send
    if(resp_callback != NULL) { 

      //we made the connection so lets save the resp_callback
      //first grab a lock to modify the map
      this->call_backs_mutex.lock();

      this->call_backs[sfd][uuid_str] = resp_callback;

      //unlock access to the map
      this->call_backs_mutex.unlock();

    }     

    //now let's make a msg frame

    //make a buffer just big enough for uuid + size + del
    uint32_t mfs = 37+size+1;
    char msg_frame[mfs]; 
    SocketUtils::pack_frame(uuid_str.c_str(), data, size, msg_frame);

    //grab a global write lock
    this->w_mutex.lock();

    //let's get our buffered writer out for this sfd
    std::unordered_map<int32_t, SocketUtils::WriteR>::const_iterator wr_got = this->wrm.find(sfd);
    if( wr_got == this->wrm.end()) {

      //if we don't have an entry lets create one
      struct SocketUtils::WriteR nwr;
      nwr.mutex = new std::mutex();
      nwr.is_valid = true;
      this->wrm.emplace(sfd, std::move(nwr));

    }

    SocketUtils::WriteR *wr = &this->wrm.find(sfd)->second;
    //release the global write lock
    this->w_mutex.unlock();

    //grab the sfd write lock
    wr->mutex->lock();

    if(wr->is_valid) {

      //if this resource is not dead then write to the buffered writer 
      wr->bw.write(msg_frame, mfs);

    } else {

      //could not write hte data
      result = false;

    }

    //release the sfd write lock
    wr->mutex->unlock();

    if(result) {

      //if buffered writer was valid then we have data to write

      //tell empoll to let us know when we can write cause we have stuff to write
      e_mutex.lock();
      sfd_events[sfd] |= EPOLLOUT;
      SocketUtils::set_epoll(ep_sfd, sfd, sfd_events[sfd] );
      e_mutex.unlock();

    }

  }

  return result;

}

/**
 * Sends a message on to the node that the hash_key hashes to.  It will block and put response in the result
 */
bool SocketClient::send_msg(const char *data, size_t size, std::string &hash_key, std::vector<char> &result, uint64_t to_millis) {

  bool success = false;
  std::mutex sm_mutex;
  std::unique_lock<std::mutex> lck(sm_mutex, std::defer_lock);
  std::condition_variable cv;
  bool ss = false;
  
  std::function<void(std::vector<char>)> call_back = [&success, &result, &cv, &ss](std::vector<char> da_result) {

    //set the result and notify
    success = true;
    result = da_result;
    ss = true;
    cv.notify_one();

  };

  success = send_msg(data, size, hash_key, call_back);

  if(success) {

    //if success was false right off the bat that means there was no node to send it to
    //so we don't get into this block

    //init the wait return value to no timeout
    std::cv_status w_status = std::cv_status::no_timeout;

    //wait here untill callback is called or we timeout
    lck.lock();
    while(!ss && w_status == std::cv_status::no_timeout) {

      w_status = cv.wait_for(lck, std::chrono::milliseconds(to_millis));

    }
    lck.unlock();

  }

  return success;

}

/**
 * Closes the server sfd as well as cleans up
 */
void SocketClient::close_n_clean(int32_t ep_sfd, int32_t sfd) {

  //clean conns
  this->conn_mutex.lock();
  std::string node = this->conns[sfd];
  this->conns.erase(sfd);
  this->conn_mutex.unlock();

  logger.error(std::string("closing socket: ") + std::to_string(sfd) + std::string(" on host: ") + node);

  close(sfd);
  close(ep_sfd);


  //clean callbacks
  this->call_backs_mutex.lock();
  this->call_backs.erase(sfd);
  this->call_backs_mutex.unlock();

  //clean with read resources
  this->r_mutex.lock();
  std::unordered_map<int32_t, SocketUtils::ReadR>::const_iterator rr_got = this->rrm.find(sfd);
  if( rr_got != this->rrm.end()) {

    delete this->rrm[sfd].mutex;
    this->rrm.erase(sfd);
  }
  this->r_mutex.unlock();

  //clean the write resources
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

}

/**
 * Method adds sfd to a set of zombied to be reaped later
 */
void SocketClient::a_zombied(int32_t sfd) {

  this->conn_mutex.lock();
  std::string node = this->conns[sfd];
  this->conn_mutex.unlock();

  this->hs_mutex.lock();
  //unset healthy on host status
  this->h_status[node].is_healthy = false;
  this->hs_mutex.unlock();

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
void SocketClient::reap_resources() {

  logger.info("Starting zombied resource reaper");

  while(1) {

    //sleep for 30 seconds and then loop
    std::this_thread::sleep_for(std::chrono::seconds(30));
    logger.info("Reaping zombied resources");

    //grab the zombied sfd lock
    this->z_mutex.lock();

    //these are the zombies that need to be removed
    std::vector<int32_t> zv;
    for(auto it = this->zm.begin(); it != this->zm.end(); ++it ) {

      //reap after 10 seconds of being marked for death
      if((Utils::epoch_millis_now() - it->second) > 10000) {

        int32_t sfd = it->first;

        //get host name
        this->conn_mutex.lock();
        std::string node = this->conns[sfd];
        this->conn_mutex.unlock();

        int32_t ep_sfd = this->h_status[node].ep_sfd;

        //close and clean resources
        this->close_n_clean(ep_sfd, sfd);
        zv.emplace_back(sfd);

        //attempt reconnect
        if(!make_connection(node)) {

          //add this host to the failed list to reattempt later
          this->eh_mutex.lock();
          this->e_hosts.push_back(node);
          this->eh_mutex.unlock();

        }

      }

    }

    for(int32_t &zsfd : zv) {

      //now remove the entries from our zombie set
      this->zm.erase(zsfd);

    }

    this->z_mutex.unlock();

  }

}


