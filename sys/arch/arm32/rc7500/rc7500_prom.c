/*	$NetBSD: rc7500_prom.c,v 1.3 2000/03/06 21:36:06 thorpej Exp $	*/

/* 
 * Copyright (c) 1992, 1994, 1995 Carnegie Mellon University
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

#include <sys/param.h>

#include <arm32/dev/rc7500_prom.h>

#include <dev/cons.h>

/* XXX this is to fake out the console routines, while booting. */
void promcnputc __P((dev_t, int));
int promcngetc __P((dev_t));
struct consdev promcons = { NULL, NULL, promcngetc, promcnputc,
			    nullcnpollc, NULL, makedev(23,0), 1 };

void
init_prom_interface()
{
	/* XXX fake out the console routines, for now */
	cn_tab = &promcons;
}

/*
 * promcnputc:
 *
 * Remap char before passing off to prom.
 *
 * Prom only takes 32 bit addresses. Copy char somewhere prom can
 * find it. This routine will stop working after pmap_rid_of_console 
 * is called in alpha_init. This is due to the hard coded address
 * of the console area.
 */
void
promcnputc(dev, c)
	dev_t dev;
	int c;
{
	prom_putchar(c);
}

/*
 * promcngetc:
 *
 * Wait for the prom to get a real char and pass it back.
 */
int
promcngetc(dev)
	dev_t dev;
{
        int ret;

        for (;;) {
		ret = prom_getchar();
                if (ret != -1)
                        return(ret);
        }
}

/*
 * promcnlookc:
 *
 * See if prom has a real char and pass it back.
 */
int
promcnlookc(dev, cp)
	dev_t dev;
	char *cp;
{
        int ret;

	ret = prom_getchar();
	if (ret != -1) {
		*cp = ret;
		return 1;
	} else
		return 0;
}
