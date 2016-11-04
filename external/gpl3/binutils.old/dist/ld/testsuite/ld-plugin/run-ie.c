#include <stdio.h>

extern void abort (void);

__thread int tls_ie __attribute__((tls_model("initial-exec"))) = 4;

int get_ie (void)
{
  return tls_ie;
}

int *get_iep (void)
{
  return &tls_ie;
}

int main (void)
{
  int val;

  val = get_ie ();
  if (val != 4)
    abort ();

  val = *get_iep ();
  if (val != 4)
    abort ();

  printf ("IE: %d\n", val);

  return 0;
}
