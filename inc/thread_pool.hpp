#ifndef AS_UTILS_THREAD_POOL_HPP
#define AS_UTILS_THREAD_POOL_HPP

#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>

namespace asutils {

  class ThreadPool {

    private:

      /**
       * conditional variable to wait/notify on for the 
       * producer
       */
      std::condition_variable p_cv;

      /**
       * conditional variable to wait/notify on for the 
       * consumer
       */
      std::condition_variable c_cv;


      /**
       * A mutex to lock critical sections
       */
      std::mutex mutex;

      /**
       * A queue that will hold the work
       */
      std::queue<std::function<void()>> w_queue;

      /**
       * The desired size of this pool
       */
      uint32_t p_size;


      /**
       * This method creates the threads that will live forever and 
       * do work off of the w_queue
       */
      void build_worker_threads();

      void do_work();

    public:

      /**
       * The constructor adds a threadpool size
       */
      ThreadPool(const uint32_t p_size);

      /**
       * Adds work to be done
       */
      void add_work(const std::function<void()> work); 

  };

}

#endif
