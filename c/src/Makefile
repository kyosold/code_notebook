CC = gcc
CFLAGS = -g

AR = ar
ARFLAGS = rcv

INCS = -I/usr/include
LDS = -L/usr/lib64 -luuid

all: libsc.so libsc.a libsc_example

OBJS = array/array.o \
		log/slog.o \
		code/base64.o code/crc32.o code/md5.o code/quote_print.o \
		code/sha1.o code/uniqid.o \
		schar/schar.o

libsc.so: $(OBJS)
	$(CC) -O -fPIC -shared -o libsc.so $(OBJS)

libsc.a: $(OBJS)
	$(AR) $(ARFLAGS) libsc.a $(OBJS)

libsc_example: libsc_example.c libsc.a
	$(CC) $(CFLAGS) -o libsc_example libsc_example.c libsc.a

%.o:%.c
	$(CC) $(CFLAGS) -c -fPIC -o $@ $^


clean:
	rm -f libsc.so
	rm -f libsc.a
	rm -f $(OBJS)
	rm -f libsc_example
