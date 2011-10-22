
#include "header.h"

namespace JepJep {

class Jotain
{
public:
  Jotain();
  virtual ~Jotain();

  int m_value;

  static int muumi(int argumentti) {int sisa=argumentti+1;};

  int toinenfunc(int arg) {m_value=arg*2;}

};


struct Toinen {
  int arvo;
};

Jotain::Jotain() {
  
}

}

int main(int argc, char *argv[])
{
  JepJep::Jotain j;
  j.m_value=15;
  
  return 0;
}

    void f() {
      struct { int a; } s;
      int b;
      s.a=1;
    }
