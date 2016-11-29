#ifndef AS_UTILS_BUFFERED_WRITER_HPP
#define AS_UTILS_BUFFERED_WRITER_HPP

#include <vector>
#include <functional>

namespace asutils {

  class BufferedWriter {

    private:

      /**
       * The buffer to pool the message frame bytes
       */
      std::vector<char> buffer;

    public:

      /**
       * Default constructor
       */
      BufferedWriter();

      /**
       * This method will add to this buffer
       */
      void write(const char *msg_frame, size_t size);

      /**
       * This method will return a chunk of the buffer without removing it
       */
      void iread(char *r_buffer, size_t size);

      /**
       * This method clears this many bytes from the buffer
       */
      void clear(size_t size);

      /**
       * This method returns the current size of the buffer
       */
      size_t size();
  };

}

#endif


