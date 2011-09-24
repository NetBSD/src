namespace M
{
  int x = 911;
}

namespace N
{
  int x = 912;
}

int marker10 ()
{
  using namespace M;
  int y = x + 1; // marker10 stop
  using namespace N;
  return y;
}

namespace J
{
  int jx = 44;
}

namespace K
{
  int marker9 ()
  {
    //x;
    return marker10 ();
  }
}

namespace L
{
  using namespace J;
  int marker8 ()
  {
    jx;
    return K::marker9 ();
  }
}

namespace G
{
  namespace H
  {
    int ghx = 6;
  }
}

namespace I
{
  int marker7 ()
  {
    using namespace G::H;
    ghx;
    return L::marker8 ();
  }
}

namespace E
{
  namespace F
  {
    int efx = 5;
  }
}

using namespace E::F;
int marker6 ()
{
  efx;
  return I::marker7 ();
}

namespace A
{
  int _a = 1;
  int x = 2;
}

namespace C
{
  int cc = 3;
}

namespace D
{
  int dx = 4;
}

using namespace C;
int marker5 ()
{
  cc;
  return marker6 ();
}

int marker4 ()
{
  using D::dx;
  return marker5 ();
}

int marker3 ()
{
  return marker4 ();
}

int marker2 ()
{
  namespace B = A;
  B::_a;
  return marker3 ();
}

int marker1 ()
{
  int total = 0;
    {
      int b = 1;
        {
          using namespace A;
          int c = 2;
            {
              int d = 3;
              total = _a + b + c + d + marker2 (); // marker1 stop
            }
        }
    }
  return marker2 () + total;
}

int main ()
{
  using namespace A;
  _a;
  return marker1 ();
}
