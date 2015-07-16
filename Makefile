all: bindiff binpatch

clean:
	rm -f bindiff binpatch *.o

bindiff: bindiff.c
	gcc -lssl -lcrypto -o bindiff bindiff.c

binpatch: binpatch.c
	gcc -lssl -lcrypto -o binpatch binpatch.c
