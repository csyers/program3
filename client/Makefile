all: client

client: client.c
	gcc client.c -Wall -lmhash -lssl -o client

.PHONY: clean

clean:
	-@rm -rf client 2>/dev/null
