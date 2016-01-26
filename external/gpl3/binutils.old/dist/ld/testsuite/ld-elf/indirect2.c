extern void foo (void);

asm (".symver foo,foo@@@FOO");

void
bar (void)
{
  foo ();
}
