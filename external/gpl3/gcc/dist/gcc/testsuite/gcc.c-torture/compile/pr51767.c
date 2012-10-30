/* PR rtl-optimization/51767 */

extern void fn1 (void), fn2 (void);

static inline __attribute__((always_inline)) int
foo (int *x, long y)
{
  asm goto ("" : : "r" (x), "r" (y) : "memory" : lab);
  return 0;
lab:
  return 1;
}

void
bar (int *x)
{
  if (foo (x, 23))
    fn1 ();
  else
    fn2 ();

  foo (x, 2);
}
