#include <default_handler.hpp>

using namespace asutils;

log4cpp::Category& DefaultHandler::logger = log4cpp::Category::getRoot();

int DefaultHandler::handle(HttpRequest& request, HttpResponse& response) {

	build_response(request, response);
	
	response.write_data(response.get_data_buffer().c_str(), response.get_data_buffer().size(), 200, false);

  return MHD_YES;

}

int DefaultHandler::handle_streaming_data(HttpRequest& request, HttpResponse& response, std::string filename, std::string content_type,
				 	std::string transfer_encoding, const char *data, uint64_t off, size_t size) {

	if(off == 0) {

		//if off is 0 then this is the firt call
		build_response(request, response);
		response.add_to_buffer("\n\n POST data:\n");

	} 

  if(data != NULL)
    response.add_to_buffer(data);

	if(!size) {

		//if size is 0 this is the last call
		response.write_data(response.get_data_buffer().c_str(), response.get_data_buffer().size(), 200, false);

	}

  return MHD_YES;

}

void DefaultHandler::build_response(HttpRequest& request, HttpResponse& response) {

	response.add_header("Content-Type", "text/html");

	//lets build a output
	std::string data("libcpphttpd is alive yo! here is what you sent me bro...\n\n");

	std::unordered_map<std::string, std::string> args = request.get_get_args();
	
	data += "\nHEADERS and GET args:\n";

	for ( auto it = args.begin(); it != args.end(); ++it ) {

    data +=  " " + it->first + ":" +  it->second + "\n";

	}

	response.add_to_buffer(data);

}

