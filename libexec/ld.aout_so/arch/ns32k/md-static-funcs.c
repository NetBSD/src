/*	$NetBSD: md-static-funcs.c,v 1.3 2005/12/24 22:04:11 perry Exp $	*/

/*
 * Called by ld.so when onanating.
 * This *must* be a static function, so it is not called through a jmpslot.
 */

#include <sys/syscall.h>
#define write(fd, s, n)		__syscall(SYS_write, (fd), (s), (n))
#define _exit(n)		__syscall(SYS_exit, (n))

__asm("___syscall:");
__asm("	movd tos,r1");		/* return address */
__asm("	movd tos,r0");		/* syscall number */
__asm("	movd r1,tos");
__asm("	svc");			/* do system call */
__asm("	bcc 1f");		/* check error */
__asm("	movqd -1,r0");
__asm("1:	jump 0(0(sp))");	/* return */

static void
md_relocate_simple(r, relocation, addr)
struct relocation_info	*r;
long			relocation;
char			*addr;
{
    if (r->r_relative) {
    	if (r->r_disp != 2) {
    		write(2, "Illegal runtime relocation for ld.so",
		      sizeof("Illegal runtime relocation for ld.so") - 1);
		_exit(1);
    	}
	*(long *)addr += relocation;
    }
}

#undef _exit
#undef write
