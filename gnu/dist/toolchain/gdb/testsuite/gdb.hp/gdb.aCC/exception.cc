// Test file for exception handling support.

#include <iostream.h>

int foo (int i)
{
  if (i < 32)
    throw (int) 13;
  else
    return i * 2;
}

extern "C" int bar (int k, unsigned long eharg, int flag);
    
int bar (int k, unsigned long eharg, int flag)
{
  cout << "k is " << k << " eharg is " << eharg << " flag is " << flag << endl;
  return 1;
}

int main()
{
  int j;

  try {
    j = foo (20);
  }
  catch (int x) {
    cout << "Got an except " << x << endl;
  }
  
  try {
    try {
      j = foo (20);
    }
    catch (int x) {
      cout << "Got an except " << x << endl;
      throw;
    }
  }
  catch (int y) {
    cout << "Got an except (rethrown) " << y << endl;
  }

  // Not caught 
  foo (20);
  
}
