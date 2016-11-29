#include "gtest/gtest.h"
#include "socket_utils.hpp"
#include <uuid/uuid.h> 
#include "utils.hpp"

using namespace asutils;

TEST(SocketUtils, TestMsgPackUnpack) {

  std::string uuid_str = Utils::build_uuid_str();

  std::string msg_s = "I really love apples";

  uint32_t mfs = 37+msg_s.size()+1;
  char msg_frame[mfs];

  SocketUtils::pack_frame(uuid_str.c_str(), msg_s.c_str(), msg_s.size(), msg_frame);

  std::vector<char> uuid_v;
  std::vector<char> msg_v;

  SocketUtils::unpack_frame(msg_frame, mfs, uuid_v, msg_v);

  //let's confirm uuid
  for(uint32_t i=0; i < uuid_v.size(); ++i) {

    ASSERT_EQ(uuid_str[i], uuid_v[i]);

  }

  //now let's confirm message
  for(uint32_t i=0; i < msg_v.size(); ++i) {

    ASSERT_EQ(msg_s[i], msg_v[i]);

  }

}
