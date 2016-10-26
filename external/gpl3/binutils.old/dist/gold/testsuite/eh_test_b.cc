#include <iostream>
#include <cstdlib>

void
foo()
{
}

int
main()
{
  try
    {
      throw(1);
    }
  catch(int)
    {
      std::cout << "caught" << std::endl;
      exit(0);
    }
  std::cout << "failed" << std::endl;
  exit(1);
}
