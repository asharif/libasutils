#ifndef AS_UTILS_DEFAULT_HANDLER_HPP
#define AS_UTILS_DEFAULT_HANDLER_HPP

#include <http_request_handler.hpp>
#include <log4cpp/Category.hh>

namespace asutils {

	class DefaultHandler : public HttpRequestHandler {

		private:

      static log4cpp::Category &logger;

			void build_response(HttpRequest& request, HttpResponse& response);

		public:

		int handle(HttpRequest& request, HttpResponse& response);

		int handle_streaming_data(HttpRequest& request, HttpResponse& response, std::string filename, std::string content_type,
				 	std::string transfer_encoding, const char *data, uint64_t off, size_t size);


	};

}

#endif
