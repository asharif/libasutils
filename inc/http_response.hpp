#ifndef AS_UTILS_HTTP_RESPONSE_HPP
#define AS_UTILS_HTTP_RESPONSE_HPP

#include <string.h>
#include <unordered_map>
#include <microhttpd.h>

namespace asutils {

	class HttpResponse {

		private:

			/**
			 * The headers of the response
			 */
			std::unordered_map<std::string, std::string> headers;

			/**
			 * Optional buffer body of response
			 */
			std::string data_buffer;

			struct MHD_Connection *connection;

			/**
			 * The MDH result; MHD_YES, MHD_NO
			 */
			int mhd_result;


		public:

			HttpResponse();
			HttpResponse(struct MHD_Connection *connection);
			~HttpResponse();

			void add_header(std::string key, std::string value);
			void add_to_buffer(std::string data);
			std::string get_data_buffer();
			void write_data(const char data[], size_t size, uint32_t status_code, bool is_error);
			int get_mhd_result();

	};

}

#endif
