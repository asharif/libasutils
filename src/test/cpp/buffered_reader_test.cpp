#include "gtest/gtest.h"
#include "buffered_reader.hpp"
#include <stdlib.h>
#include <functional>

using namespace asutils;

TEST(BufferedReader, TestBufferedReader) {

  const char *data1 =  (char*)"abc";
  const char *data2 = (char*)"def\n";
  const char *data3 = (char*)"hij\n";

  uint8_t count = 0;
  std::function<void(std::vector<char>)> call_back = [&count](std::vector<char> bytes) { 
    
    std::string line(bytes.begin(), bytes.end());

    if(count == 0)
      ASSERT_EQ(0, line.compare("abcdef"));
    else
      ASSERT_EQ(0, line.compare("hij"));

    count++;

  };
  
  BufferedReader br('\n', call_back);
  br.read(data1, 3);
  br.read(data2, 4);
  br.read(data3, 4);

}

TEST(BufferedReader, TestBufferedReaderLoop) {

  std::function<void(std::vector<char>)> call_back = [](std::vector<char> bytes) { 
    
    std::string line(bytes.begin(), bytes.end());
    ASSERT_EQ(0, line.compare("abc"));

  };
  
  BufferedReader br('\n', call_back);
  for(uint32_t i=0; i < 1000; i++) {

    br.read((char*)"abc\n", 4);

  }

}
