all: myftpd

myftpd: myftpd.c
	gcc myftpd.c -Wall -lmhash -lssl -o myftpd

.PHONY: clean

clean:
	-@rm -rf myftpd 2>/dev/null
