CC = gcc
CFLAGS = -g
INCS = -I/usr/include
LDS = -L/usr/lib64 -lcrypto -lssl

SHLD = $(CC) $(CFLAGS)

all: gpkeys

gpkeys: gpkeys.o scrypto.o
	$(SHLD) -o gpkeys gpkeys.o scrypto.o $(INCS) $(LDS)

%.o:%.c
	$(SHLD) -c $< $(INCS)

clean:
	rm -f *.o
	rm -f gpkeys