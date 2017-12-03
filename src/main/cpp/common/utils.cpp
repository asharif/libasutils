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

    if(dt.size() == 2) {

      struct tm t = {0};
      //get the date parts
      std::string dp_s = dt[0];
      std::vector<std::string> dd = Utils::split(dp_s, '-');
      if(dd.size() == 3) {

        std::string yp_s = dd[0];
        t.tm_year = std::stoi(yp_s) - 1900;
        std::string mp_s = dd[1];
        t.tm_mon = std::stoi(mp_s) - 1;
        std::string dyp_s = dd[2];
        t.tm_mday = std::stoi(dyp_s);

        //get time and millis
        std::string mtp_s = dt[1];
        std::vector<std::string> tm = Utils::split(mtp_s, ',');

        if(tm.size() == 2) {

          std::string millis_s = tm[1];
          uint32_t millis = std::stoi(millis_s);

          //now get the regular time parts
          std::string rtp_s = tm[0];
          std::vector<std::string> tt = Utils::split(rtp_s, ':');
          if(tt.size() == 3) {

            std::string rtph_s = tt[0];
            t.tm_hour = std::stoi(rtph_s); 
            std::string rtpm_s  = tt[1];
            t.tm_min = std::stoi(rtpm_s); 
            std::string rtps_s = tt[2];
            t.tm_sec = std::stoi(rtps_s); 
            t.tm_gmtoff = 0;

            time_t epoch_s = timegm(&t);

            result = (uint64_t)epoch_s * 1000 + millis;

          }

        }

      }

    }

  } catch(std::exception &e) {

    logger.error(std::string("Could not convert epoch to millis!: ") + std::string(e.what()));

  }

  return result;

}

/**
 * This method compares two floats
 */
int8_t Utils::compare(const float a, const float b) {

  int8_t result = 0;
  float epsilon = 0.0000001;

  if(std::abs(a-b) > epsilon) {


    if(a < b) {

      result = -1;

    } else {

      result = 1;

    }

  }

  return result;

}

/**
 * This method compares two doubles
 */
int8_t Utils::compare(const double a, const double b) {

  int8_t result = 0;
  double epsilon = 0.0000000000000001;

  if(std::abs(a-b) > epsilon) {

    if(a < b) {

      result = -1;

    } else {

      result = 1;

    }

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

/**
 * This function executes a command on the system through pipes and returns the standard output
 * int a vector representing each line
 */
std::vector<std::string> Utils::exec(std::string cmd) {

  //convert c++ string to c string for c pipes
  const char* cmd_carr = cmd.c_str();

  //the result of the call line by line
  std::vector<std::string> r_lines;

  //a callback for our super sweet BufferedReader
  std::function<void(std::vector<char>)> cb = [&r_lines](std::vector<char> line_chars) {

    //each line we get let's make a string out of the vector and put it into a result
    //vector
    r_lines.push_back(std::string(line_chars.begin(), line_chars.end()));

  };

  //our sweet buffered reader
  BufferedReader br('\n', cb);

  //128 bytes for our read off the pipe
  char buffer[128];

  //a shared pointer to get the pipe and closes the pipe when this stack frame returns
  std::shared_ptr<FILE> pipe(popen(cmd_carr, "r"), pclose);

  if (!pipe) {

    //uh oh
    logger.error("popen failed!\n");

  } else {

    while (!feof(pipe.get())) {

      //read until not eof since fgets will stop on a new line

      if (fgets(buffer, 128, pipe.get()) != nullptr){

        //read 128 bytes at a time or until you see a new line

        //here we don't know if we've read 128 bytes so we gotta check 
        //how many bytes we've read to pass to our BufferedReader
        int32_t s = 0;
        for(int32_t i=0; i < 128; i++) {

          //as long as we dont' have the null byte increment s otherwise break
          if(buffer[i] != '\0')
            s++;
          else 
            break;
        }

        //push it into buffered reader
        br.read(buffer, s);

      }

    }

  }

  //return the vector of lines that were read after executing the command
  return r_lines;

}

