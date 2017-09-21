SHELL=/bin/bash

VERSION=local

#main
COMMON_SRC=`pwd`/src/main/cpp/common/**
SSERVER_SRC=$(COMMON_SRC) `pwd`/src/main/cpp/sserver/**
SCLIENT_SRC=$(COMMON_SRC) `pwd`/src/main/cpp/sclient/**
HTTP_SERVER_SRC=$(COMMON_SRC) `pwd`/src/main/cpp/httpserver/**
INC=-I `pwd`/src/main/hpp/ -I `pwd`/inc/ -I $(BOOST_CPP_HOME)
COMMON_SHARED_LIB=-pthread $(LIB_UUID_HOME)/lib/libuuid.so $(LIB_MICROHTTP_HOME)/lib/libmicrohttpd.so $(BOOST_CPP_HOME)/stage/lib/libboost_program_options.so $(LIB_LOG4CPP_HOME)/lib/liblog4cpp.so
COMMON_STATIC_LIB=$(LIB_UUID_HOME)/lib/libuuid.a $(LIB_MICROHTTP_HOME)/lib/libmicrohttpd.a $(BOOST_CPP_HOME)/stage/lib/libboost_program_options.a $(LIB_LOG4CPP_HOME)/lib/liblog4cpp.a
SSERVER_BIN=bin/sserver
SCLIENT_BIN=bin/sclient
HTTP_SERVER_BIN=bin/hserver
SHARED_LIB=lib/libasutils-$(VERSION).so
STATIC_LIB=lib/libasutils-$(VERSION).a

#test
TEST_SRC=`pwd`/src/test/cpp/**
TEST_INC=-I $(GTEST_HOME)/include/ 
TEST_STATIC_LIB=$(GTEST_HOME)/build/libgtest.a
TEST_BIN=bin/asutils-test
TEST_OBJ=tmp/obj/

LINUX=`uname`

.PHONY: sserver, sserver-debug, sserver-obj, sserver-obj-debug, sclient, sclient-debug, sclient-obj, sclient-obj-debug, hserver, hserver-debug, hserver-obj, hserver-obj-debug, lib, lib-debug, lib-static, lib-static-debug

default:
	@echo "No default target"

sserver: clean-obj dir sserver-obj
	@g++-5 -std=c++14 -o $(SSERVER_BIN) tmp/obj/**  $(COMMON_STATIC_LIB)
	@rm -rf tmp/

sserver-debug: clean-obj dir sserver-obj-debug
	@g++-5 -std=c++14 -g -o $(SSERVER_BIN) tmp/obj/** $(COMMON_STATIC_LIB) 
	@rm -rf tmp/

sserver-obj:
	@g++-5 -std=c++14 -Wall -Ofast -c $(INC) $(SSERVER_SRC) 
	@mv *.o tmp/obj/

sserver-obj-debug:
	@g++-5 -std=c++14 -Wall -g -c $(INC) $(SSERVER_SRC) 
	@mv *.o tmp/obj/

sclient: clean-obj dir sclient-obj
	@g++-5 -std=c++14 -o $(SCLIENT_BIN) tmp/obj/** $(COMMON_STATIC_LIB)
	@rm -rf tmp/

sclient-debug: clean-obj dir sclient-obj-debug
	@g++-5 -std=c++14 -g -o $(SCLIENT_BIN) tmp/obj/** $(COMMON_STATIC_LIB) 
	@rm -rf tmp/

sclient-obj:
	@g++-5 -std=c++14 -Wall -Ofast -c $(INC) $(SCLIENT_SRC) 
	@mv *.o tmp/obj/

sclient-obj-debug:
	@g++-5 -std=c++14 -Wall -g -c $(INC) $(SCLIENT_SRC) 
	@mv *.o tmp/obj/

hserver: clean-all dir hserver-obj
	@g++-5 -std=c++14 -o $(HTTP_SERVER_BIN) tmp/obj/** $(COMMON_STATIC_LIB)
	@rm -rf tmp/

hserver-debug: clean-all dir hserver-obj-debug
	@g++-5 -std=c++14 -o $(HTTP_SERVER_BIN) tmp/obj/** $(COMMON_STATIC_LIB)
	@rm -rf tmp/

hserver-obj:
	@g++-5 -std=c++14 -D$(LINUX) -Wall -Ofast -c $(INC) $(COMMON_SRC) $(HTTP_SERVER_SRC)
	@mv *.o tmp/obj/

hserver-obj-debug:
	@g++-5 -std=c++14 -D$(LINUX) -Wall -g -c $(INC) $(COMMON_SRC) $(HTTP_SERVER_SRC)
	@mv *.o tmp/obj/

lib-shared: clean-all dir
	@g++-5 -std=c++14 -Wall -c -fpic -Ofast $(INC) $(COMMON_SRC)
	@mv *.o tmp/obj/
	@g++-5 -shared -o $(SHARED_LIB) tmp/obj/** 

lib-shared-debug: clean-all dir
	@g++-5 -std=c++14 -Wall -g -c -fpic $(INC) $(COMMON_SRC)
	@mv *.o tmp/obj/
	@g++-5 -shared -o $(SHARED_LIB) tmp/obj/** 

lib-static: clean-all dir
	@cp $(COMMON_STATIC_LIB) tmp/obj/
	@cd tmp/obj/; for i in `ls`; do ar -x $$i; done; rm *.a
	@g++-5 -std=c++14 -Wall -c -fpic -Ofast $(INC) $(COMMON_SRC)
	@mv *.o tmp/obj/
	@ar -cr $(STATIC_LIB) tmp/obj/*.o

lib-static-debug: clean-all dir
	@cp $(COMMON_STATIC_LIB) tmp/obj/
	@cd tmp/obj/; for i in `ls`; do ar -x $$i; done; rm *.a
	@g++-5 -std=c++14 -Wall -g -c -fpic $(INC) $(COMMON_SRC)
	@mv *.o tmp/obj/
	@ar -cr $(STATIC_LIB) tmp/obj/*.o

#Test targets

test: clean-all dir sserver-obj-debug  test-obj
	@g++-5 -g -o $(TEST_BIN) tmp/obj/** $(TEST_STATIC_LIB) $(COMMON_SHARED_LIB) 
	@$(TEST_BIN)

test-obj:
	@mkdir -p $(TEST_OBJ) 
	@g++-5 -std=c++14 -Wall -g -c $(TEST_INC) $(INC) $(TEST_SRC)
	@mv *.o tmp/obj/

dir:
	@mkdir -p bin/
	@mkdir -p tmp/obj/
	@mkdir -p lib/

clean-obj:
	@rm -rf tmp/

clean-all: clean-obj
	@rm -rf bin/**
	@rm -rf lib/**
	@rm -rf *.o

