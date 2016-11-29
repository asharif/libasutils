#include <iostream>
#include <default_handler.hpp>
#include <boost/program_options.hpp>
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <http_server.hpp>

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 8080
#endif

#ifndef DEFAULT_THREAD_POOL_SIZE
#define DEFAULT_THREAD_POOL_SIZE 4
#endif

using namespace std;
using namespace asutils;
using namespace boost::program_options;

void configure_log4cpp();

int main(int argc, char *argv[]) {

  configure_log4cpp();
	log4cpp::Category& logger = log4cpp::Category::getRoot();

	try {

		uint32_t port;
		uint32_t tp;

		//boost is overkill for this, but so clean
		options_description desc("Usage");
		desc.add_options()
			("help", "Show help message")
			("port", value<uint32_t>(&port)->default_value(DEFAULT_PORT), "Port to run http server on")
			("tp", value<uint32_t>(&tp)->default_value(DEFAULT_THREAD_POOL_SIZE), "The size of the server threadpool");

		variables_map vm;
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);

		if(vm.count("help")) {

			cout << desc << std::endl;
			exit(0);

		}

		logger.info("Starting server on port " + std::to_string(port) + ", with a threadpool size of: " + std::to_string(tp));
		//start the server on port (port), a threadpool (tp), and a buffer for upload data (1MiB)
		HttpServer server(port, tp, 1048576);
		DefaultHandler default_handler;
		server.register_handler("/", default_handler);
		server.start();
		
	} catch(exception &e) {

		logger.error(e.what());

	}

}

void configure_log4cpp() {

  std::string initFileName = "etc/log4cpp.properties";
  log4cpp::PropertyConfigurator::configure(initFileName);

}
