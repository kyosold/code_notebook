CC      = gcc
CFLAGS  = -g
INCS    = -I/usr/include/
LDS     = -L/usr/lib64/ -lssl -lcrypto
SHLD    = $(CC) $(CFLAGS)

all: example example_ssl

example: socket_io.o example.o
	$(SHLD) -o example example.o socket_io.o $(INCS) $(LDS) 

example_ssl: example_ssl.o socket_ssl_io.o
	$(SHLD) -o example_ssl example_ssl.o socket_ssl_io.o -DSSL $(INCS) $(LDS) 

example.o: example.c
	$(SHLD) -c -o example.o example.c
socket_io.o: socket_io.c
	$(SHLD) -c -o socket_io.o socket_io.c
example_ssl.o: example_ssl.c
	$(SHLD) -c -o example_ssl.o example_ssl.c -Dssl_enable $(INCS) 
socket_ssl_io.o: socket_io.c
	$(SHLD) -c -o socket_ssl_io.o socket_io.c -Dssl_enable $(INCS) 

clean:
	rm -rf *.o
	rm -rf example
	rm -rf example_ssl