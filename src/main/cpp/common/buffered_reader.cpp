#include "buffered_reader.hpp"

using namespace asutils;

BufferedReader::BufferedReader(){

}

BufferedReader::BufferedReader(const char del, std::function<void(std::vector<char>)> call_back) {

  this->del = del;
  this->call_back = call_back;

}

/**
 * This method will read up to the 'del' and call the function
 */
void BufferedReader::read(const char *data, size_t size) {

  for(size_t i=0; i < size; ++i) {

    if(data[i] != del) {

      //we don't have a line yet so keep appending
      this->buffer.push_back(data[i]);

    } else {

      //we have a frame yay, set it into the result
      this->call_back(this->buffer);
      //and clear the local buffer
      this->buffer.clear();     

    }

  }

}
