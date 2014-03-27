# MakeFile for MetaDNS PowerDNS backend module

PATH_TO_PDNS=/home/builder/pdns-3.3
INSTALL_DIR=/usr/local/lib
OUTPUT_DIR=build
SRC_DIR=src

CC=g++
CFLAGS= -I $(PATH_TO_PDNS) -I /usr/include/mysql/ -O3 -Wall -fPIC -c -std=c++11 -DBIG_JOINS=1 -fno-strict-aliasing
LDFLAGS=-lmysqlclient_r -lpthread -lz -lm -lrt -ldl libwpmetabackend.so.0 -shared
SOURCES= wpmetabackend.cc database.cc \
		hashring.cc hashcache.cc \
		crc32.cc pool.cc poolcache.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=libwpmetabackend.so

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f *.so

install:
	cp libwpmetabackend.so $(INSTALL_DIR)

uninstall:
	rm -f $(INSTALL_DIR)/libwpmetabackend.so
