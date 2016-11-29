 #include "utils.hpp" 

using namespace asutils;

log4cpp::Category& Utils::logger = log4cpp::Category::getRoot();

/**
 * This method returns epoch now in seconds
 */
uint64_t Utils::epoch_millis_now() {

  struct timeval tp;
  gettimeofday(&tp, NULL);

  //tv_sec is the seconds and usec is the microseconds
  return  tp.tv_sec * 1000 + tp.tv_usec / 1000;

}

/**
 * This method returns epoch now in seconds
 */
uint64_t Utils::epoch_micros_now() {

  struct timeval tp;
  gettimeofday(&tp, NULL);

  //tv_sec is the seconds and usec is the microseconds
  return  tp.tv_sec * 1000000 + tp.tv_usec;

}

/**
 * This method converts a string of format YYYY-MM-DD HH:MM:SS,mmm (ex. 2016-08-16 23:30:53,397)
 * to epock millis
 */
uint64_t Utils::c_epoch_millis(std::string val) {

  uint64_t result = 0;
  try {

    std::vector<std::string> dt = Utils::split(val, ' ');

    struct tm t = {0};
    //get the date parts
    std::vector<std::string> dd = Utils::split(dt[0], '-');
    t.tm_year = std::stoi(dd[0]) - 1900;
    t.tm_mon = std::stoi(dd[1]) - 1;
    t.tm_mday = std::stoi(dd[2]);

    //get time and millis
    std::vector<std::string> tm = Utils::split(dt[1], ',');
    uint32_t millis = std::stoi(tm[1]);

    //now get the regular time parts
    std::vector<std::string> tt = Utils::split(tm[0], ':');
    t.tm_hour = std::stoi(tt[0]); 
    t.tm_min = std::stoi(tt[1]); 
    t.tm_sec = std::stoi(tt[2]); 
    t.tm_gmtoff = 0;

    time_t epoch_s = timegm(&t);

    result = (uint64_t)epoch_s * 1000 + millis;

  } catch(std::exception &e) {

    logger.error(std::string("Could not convert epoch to millis!: ") + std::string(e.what()));

  }

  return result;

}

/**
 * This method compares two floats
 */
int8_t Utils::compare(const float a, const float b) {

  uint32_t r_a = a * 1000;
  uint32_t r_b = b * 1000;

  int8_t result = 0;
  if(r_a > r_b) {

    result = 1;

  } else if(r_a < r_b) {

    result = -1;

  }

  return result;

}

/**
 * This method splits based on a char
 */
std::vector<std::string> Utils::split(std::string data, char d) {

  std::vector<std::string> result;

  std::string buffer;

  for(size_t i=0; i < data.size(); ++i) {

    if(data.at(i) == d && i == data.size()-1) {

      result.push_back(buffer);
      //add in an extra one after the d
      result.push_back("");

    }else if(data.at(i) == d || i == data.size()-1) {

      if(data.at(i) != d)
        buffer += data.at(i);

      //we have a line yay, set it into the result
      result.push_back(buffer);
      //and clear the local buffer
      buffer.clear();     

    } else {
      
      //we don't have a line yet so keep appending
      buffer += data.at(i);
      
    }

  }

  return result;

}

/**
 * Builds a vector containing the uuid bytes
 */
std::vector<char> Utils::gen_uuid() {

  std::vector<char> uuid_v;

  uuid_t uuid;
  uuid_generate(uuid);

  for(uint8_t i=0; i < 16; ++i) {

    uuid_v.push_back(uuid[i]);

  }

  return uuid_v;

}

/**
 * Builds a uuid string
 */
std::string Utils::build_uuid_str() {

  uuid_t uuid;
  uuid_generate(uuid);
  char uuid_str[37];
  uuid_unparse_lower(uuid, uuid_str);

  std::string result(uuid_str, 37);
 
  return result;
}


/**
 * Creates a 8 byte hash from a string.
 */
uint32_t Utils::hash_it(std::string val) {

  uint32_t h = 0;

  for(uint32_t i=0; i < val.size(); ++i) {

    h += 31*h + val[i];

  }

  return h;

}

/**
 * This method converts a string to a int array of length 5
 */
uint8_t *Utils::sha1_byte_arr(std::string val) {

  const char *text = val.c_str();
  uint32_t hash[5]; 
  boost::uuids::detail::sha1 sha1;
  sha1.process_bytes(text, std::strlen(text));
  sha1.get_digest(hash);

  //copy into a byte array
  uint8_t *bytes = new uint8_t[20];

  for(uint8_t i =0; i < 5; ++i) {

    *bytes =  (uint8_t)(hash[i] >> 24);
    bytes++;
    *bytes =  uint8_t((hash[i] << 8) >> 24);
    bytes++;
    *bytes =  uint8_t((hash[i] << 16) >> 24);
    bytes++;
    *bytes =  uint8_t((hash[i] << 24) >> 24);
    bytes++;

  }

  bytes -= 20;

  return bytes;

}

/**
 * This method converts a string to a hex string
 */
std::string Utils::sha1_hex_str(std::string val) {

    const char *text = val.c_str();
    boost::uuids::detail::sha1 sha1;
    uint32_t hash[5];
    sha1.process_bytes(text, std::strlen(text));
    sha1.get_digest(hash);
 
    std::stringstream ss;
    for(uint8_t i=0; i < 5 ; ++i) {

      ss << std::hex << hash[i];

    }

    return ss.str();

}

