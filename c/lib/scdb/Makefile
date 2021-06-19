CC = gcc 
CFLAGS = -fPIC
AR = ar
INCS = 
LDS = 
SHLD = $(CC) $(CFLAGS)

OBJS = scdb.o scdb_make.o buffer.o utils.o

all: libscdb.a libscdb.so scdb scdb_make

libscdb.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

libscdb.so: $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS)

scdb: $(OBJS)
	$(CC) $(CFLAGS) -g -o scdb $(OBJS) -D_TEST

scdb_make: $(OBJS)
	$(CC) $(CFLAGS) -g -o scdb_make $(OBJS) -D_TEST

%.o:%.c
	$(SHLD) -c $< $(INCS)

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f *.a
	rm -f *.so
	rm -f scdb
	rm -f scdb_make