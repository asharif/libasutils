#ifndef AS_UTILS_UTILS_HPP
#define AS_UTILS_UTILS_HPP

#include <sstream>
#include <iostream>
#include <cstring>
#include <sys/time.h>
#include <exception>
#include <stdexcept>
#include <vector>
#include <uuid/uuid.h>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/sha1.hpp>
#include <log4cpp/Category.hh>
#include <memory>
#include <functional>
#include "buffered_reader.hpp"

namespace asutils {

  class Utils {

    private:

      static log4cpp::Category &logger;

    public:

      /**
       * This method returns epoch now in seconds
       */
      static uint64_t epoch_millis_now();

      /**
       * This method returns epoch now in micros
       */
      static uint64_t epoch_micros_now();

      /**
       * This method converts a string of format YYYY-MM-DD HH:MM:SS,mmm (ex. 2016-08-16 23:30:53,397)
       * to epock millis
       */
      static uint64_t c_epoch_millis(std::string val);

      /**
       * This method compares two floats
       */
      static int8_t compare(const float a, const float b); 

      /**
       * This method compares two doubles
       */
      static int8_t compare(const double a, const double b);

      /**
       * This method splits based on a char
       */
      static std::vector<std::string> split(std::string, char d);

      /**
       * Builds a vector containing the uuid bytes
       */
      static std::vector<char> gen_uuid();

      /**
       * Builds a string from uuid_t or char[16]
       */
      static std::string build_uuid_str();

      /**
       * Creates a 8 byte hash from a string
       */
      static uint32_t hash_it(std::string val);

      /**
       * This method converts a string to a byte array
       */
      static uint8_t *sha1_byte_arr(std::string val);

      /**
       * This method converts a string to a hex string
       */
      static std::string sha1_hex_str(std::string val);

      /**
       * This function executes a command on the system through pipes and returns the standard output
       * int a vector representing each line
       */
      static std::vector<std::string> exec(std::string cmd);

  };

}

#endif
