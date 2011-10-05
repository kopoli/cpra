
PROG=cpra cpra_test
SRC=cpra.c

CFLAGS=-Wall -ggdb -O0
LDFLAGS=-L/usr/lib/llvm -lclang
CC=gcc

LINKCMD=$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

all: $(PROG)

cpra: $(SRC) Makefile
	$(LINKCMD)

cpra_test: $(SRC) cpra_test.c Makefile
	$(LINKCMD) -DCPRA_TESTING

test: cpra_test
	./$<

clean:
	$(RM) $(PROG) *.o
