/*
 * CRT module for GNU C++ compiles shared libraries.
 *
 * $Id: c++rt0.c,v 1.1 1994/01/05 21:05:37 pk Exp $
 */

void (*__CTOR_LIST__[2])(void);
void (*__DTOR_LIST__[2])(void);

/* Run all the global destructors on exit from the program.  */
 
static void
__do_global_dtors(void)
{
	unsigned long nptrs = (unsigned long) __DTOR_LIST__[0];
	unsigned i;
 
	/*
	 * Some systems place the number of pointers
	 * in the first word of the table.
	 * On other systems, that word is -1.
	 * In all cases, the table is null-terminated.
	 */
 
	/* If the length is not recorded, count up to the null. */
	if (nptrs == -1)
		for (nptrs = 0; __DTOR_LIST__[nptrs + 1] != 0; nptrs++);
 
	/* GNU LD format.  */
	for (i = nptrs; i >= 1; i--)
		__DTOR_LIST__[i] ();

}

static void
__do_global_ctors(void)
{
	void (**p)(void);

	for (p = __CTOR_LIST__ + 1; *p; )
		(**p++)();
}

extern void __init() asm(".init");

void
__init(void)
{
	static int initialized = 0;

	if (!initialized) {
		initialized = 1;
		__do_global_ctors();
		atexit(__do_global_dtors);
	}

}
