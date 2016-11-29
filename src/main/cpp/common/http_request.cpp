#include <http_request.hpp>

using namespace asutils;

HttpRequest::HttpRequest() {

	has_post_data = false;

}

HttpRequest::~HttpRequest() {

}


//add or get GET or HEADER args
void HttpRequest::add_arg(std::string key, std::string value) {

	get_args[key] = value;

}

std::string HttpRequest::get_arg(std::string key) {

	return map_lookup(key, get_args);

}

std::string HttpRequest::map_lookup(std::string key, std::unordered_map<std::string, std::string> map) {

	std::string result("");
	std::unordered_map<std::string, std::string>::const_iterator got = map.find(key);
	
	if( got != map.end()) {

		//if the key exists then lets set it as a response
		result = map[key];

	}

	return result;

}

std::unordered_map<std::string, std::string> HttpRequest::get_get_args() {

	return this->get_args;

}

void HttpRequest::set_has_post_data(bool has_post_data){

	this->has_post_data = has_post_data;

}

bool HttpRequest::get_has_post_data(){

	return this->has_post_data;

}

void HttpRequest::set_post_processor(MHD_PostProcessor *post_processor){

	this->post_processor = post_processor;

}

MHD_PostProcessor* HttpRequest::get_post_processor(){

	return this->post_processor;

}

void HttpRequest::set_prop(std::string key, void* value) {

  this->props[key] = value;

}

void* HttpRequest::get_prop(std::string key) {

  void *result = NULL;

  std::unordered_map<std::string, void*>::const_iterator got = this->props.find(key);

	if( got != this->props.end()) {

    result = this->props[key];
  }

  return result;

}




