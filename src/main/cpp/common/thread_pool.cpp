#include "thread_pool.hpp"

using namespace asutils;

/**
 * The constructor adds a threadpool size
 */
ThreadPool::ThreadPool(const uint32_t p_size) {

  //set the pool size
  this->p_size = p_size;

  //build the workers
  build_worker_threads();

}

/**
 * This method creates the threads that will live forever and 
 * do work off of the w_queue
 */
void ThreadPool::build_worker_threads() {

  for(uint32_t i=0; i < this->p_size; ++i) {

    std::thread w_thread (&ThreadPool::do_work, this);
    w_thread.detach();

  }

}

/**
 * Adds work to be done
 */
void ThreadPool::add_work(const std::function<void()> work) {

  //lets grab a lock to make sure we can handle this
  std::unique_lock<std::mutex> lck(this->mutex);

  while(this->w_queue.size() == this->p_size) {

    //the queue is full so we need to wait for room to open
      
    //while no producer signal has been sent wait here.  spin to pretect
    //against spurious wakeups
    this->p_cv.wait(lck);

  }

  //add work to our work queue
  this->w_queue.push(work);

  //unlock
  lck.unlock();

  //notify the consumers that there is work to be done
  this->c_cv.notify_one();

}

/**
 * The main loop for the threads in the pool
 */
void ThreadPool::do_work() {

  while(1) {

    //lets grab a lock to synchronize the adding and removing of work
    std::unique_lock<std::mutex> lck(this->mutex);

    while(this->w_queue.empty()) {

      //the work queue is empty so we need to wait for more work to come
      
      //while no consumer signal has been sent lets wait here
      this->c_cv.wait(lck);

    }

    //dequeue the oldest work from our queue
    std::function<void()> work = this->w_queue.front();
    this->w_queue.pop();

    //unlock
    lck.unlock();

    //send a signal to the producer letting it know there is room
    this->p_cv.notify_one();

    //do the work
    work();

  }

}
