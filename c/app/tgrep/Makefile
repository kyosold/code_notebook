CC = gcc
CFLAGS = -g
INCS =
LDS = 

SHLD = $(CC) $(CFLAGS)

OBJ = tgrep

all: $(OBJ)

$(OBJ): tgrep.o utils.o
	$(SHLD) -o $(OBJ) tgrep.o utils.o

$.o: %.c
	$(SHLD) -c $<

install:
	cp $(OBJ) /usr/bin/

uninstall:
	rm -f /usr/bin/$(OBJ)

clean:
	rm -f *.o $(OBJ)