#include <http_wrapper.hpp>

using namespace asutils;

Wrapper::Wrapper(HttpServer* server, HttpRequest* request, HttpResponse* response) {

	this->server = server;
	this->request = request;
	this->response = response;

}

Wrapper::~Wrapper() {

	delete this->request;
	delete this->response;

}

HttpServer* Wrapper::get_server() {

	return this->server;

}

HttpRequest* Wrapper::get_request() {

	return this->request;
}

HttpResponse* Wrapper::get_response() {

	return this->response;

}

