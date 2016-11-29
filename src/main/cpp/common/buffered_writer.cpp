#include "buffered_writer.hpp"

using namespace asutils;

/**
 * Default constructor
 */
BufferedWriter::BufferedWriter() {
}

/**
 * This method will add to this buffer
 */
void BufferedWriter::write(const char *msg_frame, size_t size) {

  for(uint32_t i=0; i < size; ++i) {

    this->buffer.emplace_back(msg_frame[i]);

  }
}

/**
 * This method will return a chunk of the buffer without removing it
 */
void BufferedWriter::iread(char *r_buffer, size_t size) {

  for(uint32_t i=0; i < size; ++i){

    r_buffer[i] = this->buffer[i];

  }

}

/**
 * This method clears this many bytes from the buffer
 */
void BufferedWriter::clear(size_t size) {

  this->buffer.erase(this->buffer.begin(), this->buffer.begin() + size);
}

/**
 * This method returns the current size of the buffer
 */
size_t BufferedWriter::size() {

  return this->buffer.size();

}
