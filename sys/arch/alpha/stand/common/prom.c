/* $NetBSD: prom.c,v 1.14.22.1 2017/12/03 11:35:46 jdolecek Exp $ */

/*  
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <lib/libkern/libkern.h>

#include <sys/types.h>

#include <machine/prom.h>
#include <machine/rpb.h>

#include "common.h"

int console;

#if !defined(NO_GETCHAR) || !defined(NO_PUTCHAR_HALT)
static int test_getchar(int *);
#endif
static void putonechar(int c);

void
init_prom_calls(void)
{
	extern struct prom_vec prom_dispatch_v;
	struct rpb *r;
	struct crb *c;
	char buf[4];

	r = (struct rpb *)HWRPB_ADDR;
	c = (struct crb *)((u_int8_t *)r + r->rpb_crb_off);

	prom_dispatch_v.routine_arg = c->crb_v_dispatch;
	prom_dispatch_v.routine = c->crb_v_dispatch->entry_va;

	/* Look for console tty. */
	prom_getenv(PROM_E_TTY_DEV, buf, sizeof(buf));
	console = buf[0] - '0';
}

#if !defined(NO_GETCHAR) || !defined(NO_PUTCHAR_HALT)
static int
test_getchar(int *xc)
{
	prom_return_t ret;

	ret.bits = prom_dispatch(PROM_R_GETC, console);
	*xc = ret.u.retval;
	return ret.u.status == 0 || ret.u.status == 1;
}
#endif

#if !defined(NO_GETCHAR)
int
getchar(void)
{
	int c;

	for (;;) {
		if (test_getchar(&c)) {
			if (c == 3)
				halt();
			return c;
		}
	}
}
#endif

static void
putonechar(int c)
{
	prom_return_t ret;
	char cbuf = c;

	do {
		ret.bits = prom_dispatch(PROM_R_PUTS, console, &cbuf, 1);
	} while ((ret.u.retval & 1) == 0);
}

void
putchar(int c)
{
#if !defined(NO_PUTCHAR_HALT)
	int typed_c;
#endif

	if (c == '\r' || c == '\n') {
		putonechar('\r');
		c = '\n';
	}
	putonechar(c);
#if !defined(NO_PUTCHAR_HALT)
	if (test_getchar(&typed_c))
		if (typed_c == 3)
			halt();
#endif
}

int
prom_getenv(int id, char *buf, int len)
{
	/* 
	 * On at least some systems, the GETENV call requires a
	 * 8-byte-aligned buffer, or it bails out with a "kernel stack
	 * not valid halt". Provide a local, aligned buffer here and
	 * then copy to the caller's buffer.
	 */
	static char abuf[128] __attribute__((aligned (8)));
	prom_return_t ret;

	ret.bits = prom_dispatch(PROM_R_GETENV, id, abuf, 128);
	if (ret.u.status & 0x4)
		ret.u.retval = 0;
	len = min(len - 1, ret.u.retval);
	memcpy(buf, abuf, len);
	buf[len] = '\0';

	return (len);
}
