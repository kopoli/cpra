
#include "header.h"

#define ROUTAVAURIO 77

static void *jeejee;

typedef void loppu;

struct raken {
  int a,b;
};

char *variable;

void f() {
  struct { int a; } s;
  int b;
  s.a=1;
}

int main(int argc, char *argv[])
{
  struct {
    int translation;
    int sisalla;
    struct {
      int A;
    } reku;
   } status = {};

  struct raken raken_var = {.a=3, .b=14};

  const volatile char *vali;
  /* int Hamays=30; */

  f();

  status.translation=1;
  status.sisalla=status.translation*3;

  vali=3;
  
  return 0;
}
