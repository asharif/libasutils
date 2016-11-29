#ifndef AS_UTILS_HTTP_REQUEST_HPP
#define AS_UTILS_HTTP_REQUEST_HPP

#include <string.h>
#include <unordered_map>
#include <microhttpd.h>

namespace asutils {

	class HttpRequest {

		private:

			/**
			 * The get arguments of the request.  This includes get arguments as well as headers
			 */
			std::unordered_map<std::string, std::string> get_args;

			/**
			 * A helper method for looking elements up in a map without creating an entry
			 */
			std::string map_lookup(std::string key, std::unordered_map<std::string, std::string> map);

      /**
       * This map allows storing request variables.  void* is probably no good but my c++
       * is not strong enough.  @TODO: replace void* with something more c++y
       */
      std::unordered_map<std::string, void*> props;

			bool has_post_data;
			MHD_PostProcessor *post_processor;

		public:

			HttpRequest();
			~HttpRequest();

			//add or get GET args or HEADERS
			void add_arg(std::string key, std::string value);
			std::string get_arg(std::string key);
			
			std::unordered_map<std::string, std::string> get_get_args();

			void set_has_post_data(bool has_post_data);
			bool get_has_post_data();

			void set_post_processor(MHD_PostProcessor *post_processor);
			MHD_PostProcessor* get_post_processor();

      void set_prop(std::string key, void* value);
      void* get_prop(std::string key);
			
	};

}

#endif
