
PROG=cpra cpra_test
SRC=cpra.c

CFLAGS=-Wall -W -ggdb
LDFLAGS=-L/usr/lib/llvm -lclang
CC=clang

LINKCMD=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

all: $(PROG)

cpra: $(SRC)
	$(LINKCMD)

cpra_test: $(SRC)
	$(LINKCMD) -DCPRA_TESTING

test: cpra_test
	./$<

clean:
	$(RM) $(PROG) *.o
