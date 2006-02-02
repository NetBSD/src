#pragma weak bar

extern void bar ();

foo ()
{
  bar ();
}
