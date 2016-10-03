all: server

server: server.c
	gcc server.c -Wall -lmhash -lssl -o server

.PHONY: clean

clean:
	-@rm -rf server 2>/dev/null
