
PROG=cpra
SRC=cpra.c

CFLAGS=-Wall
LDFLAGS=-L/usr/lib/llvm -lclang
CC=clang

all: $(PROG)

$(PROG): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(PROG)

clean:
	$(RM) $(PROG)
