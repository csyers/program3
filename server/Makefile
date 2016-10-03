all: server

client: server.c
	gcc server.c -Wall -lmhash -lssl -o server

.PHONY: clean

clean:
	-@rm -rf searver 2>/dev/null || true
