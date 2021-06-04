CC = gcc 
CFLAGS = -g
INCS = -I/usr/local/smail_deps/openssl/include/
LDS = -L/usr/local/smail_deps/openssl/lib/ -lssl -lcrypto
SHLD = $(CC) $(CFLAGS)

OBJS = s_generate_eml smtp

all: $(OBJS)

s_generate_eml: s_generate_eml.o base64.o \
	./confparser/libconfparser.a
	$(SHLD) -o s_generate_eml s_generate_eml.o \
	base64.o \
	./confparser/libconfparser.a $(LDS)

smtp: smtp.o base64.o sio.o sio_ssl.o
	$(SHLD) -o smtp smtp.o base64.o sio.o sio_ssl.o $(LDS)

./confparser/libconfparser.a:
	cd confparser; make -f linux.mk

%.o:%.c
	$(SHLD) -c $< $(INCS)

clean:
	rm -f $(OBJS)
	rm -f *.o
	cd confparser; make -f linux.mk clean;