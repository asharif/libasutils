#include "gtest/gtest.h"
#include "thread_pool.hpp"
#include <stdlib.h>
#include <chrono>

using namespace asutils;

TEST(ThreadPool, TestParallel) {

  /*
  ThreadPool tp(4);

  for(uint32_t i=0; i < 10; i++) {

    std::function<void()> work = [i]() {

      printf("Doing work %d\n", i);

      //just block here for a bit

      for(uint64_t j=0; j < 10000000000; j++);

      printf("Done doing work %d\n", i);

    };

    tp.add_work(work);

  }

  //lets sleep a bit
  std::this_thread::sleep_for(std::chrono::seconds(60));
  */


}

