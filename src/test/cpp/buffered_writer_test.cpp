#include "gtest/gtest.h"
#include "buffered_writer.hpp"

using namespace asutils;

TEST(BufferedWriter, TestBufferedWriter) {

  const char *data1 =  (char*)"abc";
  const char *data2 = (char*)"def";
  const char *data3 = (char*)"hij";

  BufferedWriter w;

  w.write(data1, 3);
  w.write(data2, 3);

  char buffer[3];
  w.iread(buffer, 3);
  
  std::string e(data1);
  std::string r(buffer);

  ASSERT_EQ(0, e.compare(r));
  w.clear(3);

  w.write(data3, 3);

  char buffer2[6];
  w.iread(buffer2, 6);

  std::string e2 = std::string(data2) + std::string(data3);
  std::string r2(buffer2);

  ASSERT_EQ(0, e2.compare(r2));

}
