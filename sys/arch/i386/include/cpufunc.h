/*
 * Functions to provide access to special i386 instructions.
 * XXX - bezillions more are defined in locore.s but are not declared anywhere.
 *
 *	$Id: cpufunc.h,v 1.3 1993/12/20 09:08:11 mycroft Exp $
 */

#include <sys/cdefs.h>
#include <sys/types.h>

static __inline int bdb(void)
{
	extern int bdb_exists;

	if (!bdb_exists)
		return (0);
	__asm("int $3");
	return (1);
}

static __inline void
disable_intr(void)
{
	__asm __volatile("cli");
}

static __inline void
enable_intr(void)
{
	__asm __volatile("sti");
}

#ifdef notyet
void	setidt	__P((int idx, /*XXX*/caddr_t func, int typ, int dpl));
#endif
