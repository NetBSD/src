/* $NetBSD: prom.c,v 1.6 1999/03/31 03:10:00 cgd Exp $ */

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

#include <sys/types.h>

#include <machine/prom.h>
#include <machine/rpb.h>

#include "common.h"

int console;

static int test_getchar(prom_return_t *, int *);
static void putonechar(int c);

void
init_prom_calls()
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
	prom_getenv(PROM_E_TTY_DEV, buf, 4);
	console = buf[0] - '0';
}

static int
test_getchar(xret, xc)
	prom_return_t *xret;
	int *xc;
{
	xret->bits = prom_dispatch(PROM_R_GETC, console);
	*xc = xret->u.retval;
	return xret->u.status == 0 || xret->u.status == 1;
}

int
getchar()
{
	int c;
	prom_return_t ret;

	for (;;) {
		if (test_getchar(&ret, &c)) {
			if (c == 3)
				halt();
			return c;
		}
	}
}

static void
putonechar(c)
	int c;
{
	prom_return_t ret;
	char cbuf = c;

	do {
		ret.bits = prom_dispatch(PROM_R_PUTS, console, &cbuf, 1);
	} while ((ret.u.retval & 1) == 0);
}

void
putchar(c)
	int c;
{
	prom_return_t ret;
	int typed_c;

	if (c == '\r' || c == '\n') {
		putonechar('\r');
		c = '\n';
	}
	putonechar(c);
	ret.bits = prom_dispatch(PROM_R_GETC, console);
	if (ret.u.status == 0 || ret.u.status == 1)
		if (ret.u.retval == 3)
			halt();
	if (test_getchar(&ret, &typed_c))
		if (typed_c == 3)
			halt();
}

int
prom_getenv(id, buf, len)
	int id, len;
	char *buf;
{
	prom_return_t ret;

	ret.bits = prom_dispatch(PROM_R_GETENV, id, buf, len-1);
	if (ret.u.status & 0x4)
		ret.u.retval = 0;
	buf[ret.u.retval] = '\0';

	return (ret.u.retval);
}
