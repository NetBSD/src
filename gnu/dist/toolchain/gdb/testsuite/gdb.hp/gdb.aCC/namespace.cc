namespace AAA {
  char c;
  int i;
  int A_xyzq (int);
  char xyzq (char);
  class inA {
  public:
    int xx;
    int fum (int);
  };
};

int AAA::inA::fum (int i)
{
  return 10 + i;
}

namespace BBB {
  char c;
  int i;
  int B_xyzq (int);
  char xyzq (char);

  namespace CCC {
    char xyzq (char);
  };

  class Class {
  public:
    char xyzq (char);
    int dummy;
  };
};

int AAA::A_xyzq (int x)
{
  return 2 * x;
}

char AAA::xyzq (char c)
{
  return 'a';
}


int BBB::B_xyzq (int x)
{
  return 3 * x;
}

char BBB::xyzq (char c)
{
  return 'b';
}

char BBB::CCC::xyzq (char c)
{
  return 'z';
}

char BBB::Class::xyzq (char c)
{
  return 'o';
}

void marker1(void)
{
  return;
}


int main ()
{
  using AAA::inA;
  char c1;

  using namespace BBB;
  
  c1 = xyzq ('x');
  c1 = AAA::xyzq ('x');
  c1 = BBB::CCC::xyzq ('m');
  
  inA ina;

  ina.xx = 33;

  int y;

  y = AAA::A_xyzq (33);
  y += B_xyzq (44);

  BBB::Class cl;

  c1 = cl.xyzq('e');

  marker1();
  
}

  



