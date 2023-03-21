int (*func_p) (void);
extern int func (void);

void
foo (void)
{
  if (func_p != &func || func_p () != 0xbadbeef)
    __builtin_abort ();
}
