static int barx __attribute__ ((section (".data01"))) = 'b' + 'a' + 'r';

int bar (int x)
{
  if (x)
    return barx;
  else
    return 0;
}
