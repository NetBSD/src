/* Simple shared library */

int multiply(int a, int b)
{
  return a * b;
}

int square(int num)
{
  return multiply(num, num);
}
