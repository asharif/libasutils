#ifndef AS_UTILS_REQUEST_RESPONSE_WRAPPER_HPP
#define AS_UTILS_REQUEST_RESPONSE_WRAPPER_HPP

#include <http_response.hpp>
#include <http_request.hpp>
#include <http_server.hpp>

namespace asutils {

  class Wrapper {

    private:

      HttpServer *server;
      HttpRequest *request;
      HttpResponse *response;

    public:

      Wrapper(HttpServer* server, HttpRequest* request, HttpResponse* response);
      ~Wrapper();

      HttpRequest* get_request();
      HttpResponse* get_response();
      HttpServer* get_server();

  };

}

#endif
