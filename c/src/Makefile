CC = gcc
CFLAGS = -g
INCS = -I/usr/include
LDS = -L/usr/lib64 -luuid

AR = ar
ARFLAGS = rcv

all: libsc.so libsc.a 

OBJS = array/array.o \
	code/base64.o \
	code/crc32.o \
	code/md5.o \
	code/quote_print.o \
	code/sha1.o \
	code/uniqid.o \
	io/sio.o \
	log/slog.o \
	pcre/spcre.o \
	prioq/prioq.o \
	schar/schar.o \
	string/str_trim.o \
	string/str_utils.o \
	iconv/siconv.o 
	#mc/smc.o \
	#json/sjs.o \
	#mysql/smysql.o \
	#curl/scurl.o 

libsc.so: $(OBJS)
	$(CC) -O -fPIC -shared -o libsc.so $(OBJS)

libsc.a: $(OBJS)
	$(AR) $(ARFLAGS) libsc.a $(OBJS)

%.o:%.c
	$(CC) $(CFLAGS) -c -fPIC -o $@ $^

clean:
	rm -f libsc.so
	rm -f libsc.a
	rm -f array/*.o \
			code/*.o \
			curl/*.o \
			iconv/*.o \
			io/*.o \
			json/*.o \
			log/*.o \
			mc/*.o \
			mysql/*.o \
			pcre/*.o \
			prioq/*.o \
			schar/*.o \
			string/*.o


