
#include "header.h"

namespace JepJep {

class Jotain
{
public:
  Jotain();
  virtual ~Jotain();

  static int muumi(int argumentti) {int sisa=argumentti+1;};

  int m_value;

};

}

struct Toinen {
  int arvo;
};

Jotain::Jotain() {
  
}

int main(int argc, char *argv[])
{
  
  return 0;
}

    void f() {
      struct { int a; } s;
      int b;
      s.a=1;
    }
