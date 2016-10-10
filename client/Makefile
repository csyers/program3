all: myftp

myftp: myftp.c
	gcc myftp.c -Wall -lmhash -lssl -o myftp

.PHONY: clean

clean:
	-@rm -rf myftp 2>/dev/null
