#include "gtest/gtest.h"
#include "utils.hpp"

using namespace asutils;

TEST(UtilsTest, CEpochMillis) {

  uint64_t epoch_m = Utils::c_epoch_millis("2016-08-16 23:30:53,398");

  ASSERT_EQ((uint64_t)1471390253398, epoch_m);

}

TEST(UtilsTest, CEpochMillisInvalid1) {

  uint64_t epoch_m = Utils::c_epoch_millis("0");

  ASSERT_EQ((uint64_t)0, epoch_m);

}

TEST(UtilsTest, CEpochMillisInvalid2) {

  uint64_t epoch_m = Utils::c_epoch_millis("2016-08-16 23:30:53");

  ASSERT_EQ((uint64_t)0, epoch_m);

}

TEST(UtilsTest, CompareFloatEq) {

  float a = 1.256;
  float b = 1.256;

  int8_t r = Utils::compare(a, b);

  ASSERT_EQ(r, 0);

}

TEST(UtilsTest, CompareFloatG) {

  float a = 1.257;
  float b = 1.256;

  int8_t r = Utils::compare(a, b);

  ASSERT_EQ(r, 1);

}

TEST(UtilsTest, CompareFloatL) {

  float a = 1.255;
  float b = 1.256;

  int8_t r = Utils::compare(a, b);

  ASSERT_EQ(r, -1);

}

TEST(UtilsTest, CompareFloatEqG) {

  float a = 1.2565;
  float b = 1.2560;

  int8_t r = Utils::compare(a, b);

  ASSERT_EQ(r, 0);

}

TEST(UtilsTest, CompareFloatEqL) {

  float a = 1.2560;
  float b = 1.2565;

  int8_t r = Utils::compare(a, b);

  ASSERT_EQ(r, 0);

}

TEST(UtilsTest, TestSplit) {

  std::string test = "b16fd30a-ee51-4d78-badd-691ef037bfcd\t2016-09-01 19:49:47,826\tip-10-0-0-29/127.0.0.1\t0\t76.91.241.232\thttp://ads.aerserv.com/as/?plc=1010509&key=2&appversion=5.5&cb=1472759391823&dnt=0&adid=6425F59D-B25E-421B-AADC-A147101591B1&inttype=1&ip=76.91.241.232&ua=Mozilla%2F5.0+%28iPhone%3B+CPU+iPhone+OS+9_3_2+like+Mac+OS+X%29+AppleWebKit%2F601.1.46+%28KHTML%2C+like+Gecko%29+Mobile%2F13F69&model=iPhone8%2C1&osv=9.3.2&make=Apple&os=iOS&type=phone&carrier=AT%26T&lat=34.103195&long=-118.4525\tnull\tbuyer_attempt\t\t\t\t611\t1\t\t1452\t\t\t\t\t\t0\t1100\t0\tT\t1008963\t1009545\tMozilla/5.0 (iPhone; CPU iPhone OS 9_3_2 like Mac OS X) AppleWebKit/601.1.46 (KHTML, like Gecko) Mobile/13F69\t\t\t34.103195\t-118.4525\t1010509\t18\t6425F59D-B25E-421B-AADC-A147101591B1\t\t\t\t\t\t\t\t0\t1021518\t3\tVerve\t\t\t750\tfalse\t3930\t104400\t5\t22\t0\thttp://adcel.vrvm.com/htmlad?b=privaerserv3&c=999&p=iphn&adunit=mma&size=320x50&ip=76.91.241.232&lat=34.103195&long=-118.4525&ui=6425F59D-B25E-421B-AADC-A147101591B1&uis=a&site=im.talkme.talkmeim\t\tUnited States\t803\tfalse\tphone\t0\t\t1100\t\t\tnull\tOoma\tTalkatone: FREE Texts & Calls\ti - Talkatone 320x50 Tier 2\taerMarket network for PLC 1010509\tVerve_RON_320x50_iOS_1_10_Dollar\tmobile_banner_placement_type\t\t1\t0\t\tt";
  
  std::vector<std::string> v = Utils::split(test, '\t');

  ASSERT_EQ((uint32_t)77, v.size());

}

TEST(UtilsTest, TestUUID) {

  std::vector<char> uuid =  Utils::gen_uuid();
  ASSERT_EQ((uint32_t)16, uuid.size());


}


TEST(UtilsTest, TestHash) {

  std::set<uint64_t> doops;

  for(int32_t i=0; i < 1000; ++i) {

    std::string val = std::to_string(i);

    uint32_t h = Utils::hash_it(val);

    ASSERT_EQ( true, doops.find(h) == doops.end());

    doops.insert(h);

  }

}

TEST(UtilsTest, Sha1HexString) {

  std::string hash = Utils::sha1_hex_str("a");

  ASSERT_EQ("86f7e437faa5a7fce15d1ddcb9eaeaea377667b8", hash);

}

TEST(UtilsTest, Sha1ByteArray) {

  uint8_t exp[20];
  exp[0] = 0x86;
  exp[1] = 0xf7;
  exp[2] = 0xe4;
  exp[3] = 0x37;
  exp[4] = 0xfa;
  exp[5] = 0xa5;
  exp[6] = 0xa7;
  exp[7] = 0xfc;
  exp[8] = 0xe1;
  exp[9] = 0x5d;
  exp[10] = 0x1d;
  exp[11] = 0xdc;
  exp[12] = 0xb9;
  exp[13] = 0xea;
  exp[14] = 0xea;
  exp[15] = 0xea;
  exp[16] = 0x37;
  exp[17] = 0x76;
  exp[18] = 0x67;
  exp[19] = 0xb8;

  uint8_t *result = Utils::sha1_byte_arr("a");
  std::string s(result, result+20);

  for(uint8_t i=0; i < 20; ++i) {

    ASSERT_EQ( exp[i], (uint8_t)(s.at(i)));

  }

  delete[] result;

}

TEST(UtilsTest, Exec) {

  std::vector<std::string> r = Utils::exec("echo 'hello'; echo 'world'");

  ASSERT_EQ(r[0], "hello");
  ASSERT_EQ(r[1], "world");

}
