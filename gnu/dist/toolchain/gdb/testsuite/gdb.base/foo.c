static int foox __attribute__ ((section (".data00"))) = 'f' + 'o' + 'o';

int foo (int x)
{
  if (x)
    return foox;
  else
    return 0;
}
