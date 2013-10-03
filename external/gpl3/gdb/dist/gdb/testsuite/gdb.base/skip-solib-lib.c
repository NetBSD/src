/* Simple shared library */

int square(int num)
{
  return multiply(num, num);
}

int multiply(int a, int b)
{
  return a * b;
}
