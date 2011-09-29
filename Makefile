
PROG=cpra
SRC=cpra.c

CFLAGS=-Wall -W -ggdb
LDFLAGS=-L/usr/lib/llvm -lclang
CC=clang

all: $(PROG)

$(PROG): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(PROG)

clean:
	$(RM) $(PROG)
