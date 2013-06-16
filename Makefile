BIN_DIR=bin/

all : server client

server : server.c protocol.h
	gcc server.c -o ${BIN_DIR}server

client : client.c protocol.h
	gcc client.c -o ${BIN_DIR}client

clean :
	rm ${BIN_DIR}client ${BIN_DIR}server
