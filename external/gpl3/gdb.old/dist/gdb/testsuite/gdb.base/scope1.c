static int filelocal = 2;	/* In Data section */
static int filelocal_bss;	/* In BSS section */
#ifndef __STDC__
#define	const	/**/
#endif
static const int filelocal_ro = 202;	/* In Read-Only Data section */

void foo ()
{

  void bar ();
  
  static int funclocal = 3;	/* In Data section */
  static int funclocal_bss;	/* In BSS section */
  static const int funclocal_ro = 203;	/* RO Data */
  static const int funclocal_ro_bss;	/* RO Data */

  funclocal_bss = 103;
  bar ();
}

void bar ()
{
  static int funclocal = 4;	/* In data section */
  static int funclocal_bss;	/* In BSS section */
  funclocal_bss = 104;
}

void init1 ()
{
  filelocal_bss = 102;
}

/* On some systems, such as AIX, unreferenced variables are deleted
   from the executable.  On other compilers, such as ARM RealView,
   const variables without their address taken are deleted.  */
void usestatics1 ()
{
  void useit1 (const int *val);
  
  useit1 (&filelocal);
  useit1 (&filelocal_bss);
  useit1 (&filelocal_ro);
}

void useit1 (const int *val)
{
    static int usedval;

    usedval = *val;
}
