
namespace A
{
  int _a = 11;

  namespace B{

    int ab = 22;

    namespace C{

      int abc = 33;

      int second(){
        return 0;
      }

    }

    int first(){
      _a;
      ab;
      C::abc;
      return C::second();
    }
  }
}


int
main()
{
  A::_a;
  A::B::ab;
  A::B::C::abc;
  return A::B::first();
}
