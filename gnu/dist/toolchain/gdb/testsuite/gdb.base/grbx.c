static int grbxx __attribute__ ((section (".data03"))) = 'g' + 'r' + 'b' + 'x';

int grbx (int x)
{
  if (x)
    return grbxx;
  else
    return 0;
}

