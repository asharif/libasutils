#ifndef AS_UTILS_HTTP_SERVER_HPP
#define AS_UTILS_HTTP_SERVER_HPP

#include <log4cpp/Category.hh>
#include <microhttpd.h>
#include <string.h>
#include <http_response.hpp>
#include <http_request.hpp>
#include <http_request_handler.hpp>

namespace asutils {

	class HttpServer {

		private:

      static log4cpp::Category &logger;
			//the http daemon 
			struct MHD_Daemon *mhd_daemon;
			//what port we want to run the http server on
			uint32_t port;
			//the size of the threadpool
			uint32_t tpool_size;
			//the buffer size to use when reading upload (POST, etc) data
			uint32_t data_steam_buffer;

			//registered handlers for paths
			std::unordered_map<std::string, HttpRequestHandler*> endpoint_handlers;

			/**
			 * This static method is the MHD hook to handle every incoming request
			 */
			static int front_controller_c_hook(void * registered_arg, struct MHD_Connection * connection, const char * url,
														const char * method, const char * version, const char * upload_data,
														size_t * upload_data_size, void ** ptr);

			/**
			 * This static method is the libmicrohttpd hook to build the HttpRequest
			 */
			static int request_arg_builder(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);

			/**
			 * This static method will get called over and over again as post data is read off the network
			 */
			static int post_processor (void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type,
				 	const char *transfer_encoding, const char *data, uint64_t off, size_t size);
			
			/**
			 * This method routes all but POST requests to registered handlers
			 */
			int front_controller(HttpRequest* request, HttpResponse* response);

			/**
			 * This method routes POST requests to registered handlers
			 */
			int front_controller(HttpRequest* request, HttpResponse* response, const char *filename, const char *content_type,
				 	const char *transfer_encoding, const char *data, uint64_t off, size_t size);

		public:

			HttpServer(uint32_t port, uint32_t tpool_size, uint32_t data_steam_buffer);
			~HttpServer();
			uint32_t get_ds_buffer_size();
			void register_handler(std::string path, HttpRequestHandler& handler);
			void start();

	};

}

#endif
