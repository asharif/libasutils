#ifndef AS_UTILS_BUFFERED_READER_HPP
#define AS_UTILS_BUFFERED_READER_HPP

#include <vector>
#include <functional>

namespace asutils {

  class BufferedReader {

    private:

      /**
       * The buffer to pool bytes while we have not hit a frame
       */
      std::vector<char> buffer;

      /**
       * The delim to read up until to be considered a complete frame
       */
      char del;

      /**
       * The callback to invoke upon buffering a complte frame
       */
      std::function<void(std::vector<char>)> call_back;

    public:

      BufferedReader();

      /**
       * Default constructor
       */
      BufferedReader(const char del, std::function<void(std::vector<char>)> call_back);

      /**
       * This method will read up to the 'del' and call the function
       */
      void read(const char *data, size_t size);

  };

}

#endif


