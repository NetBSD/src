/*	$NetBSD: cons.c,v 1.1.8.2 2011/06/06 09:06:14 jruoho Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: cons.c 1.7 92/02/28
 *
 *	@(#)cons.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: cons.c 1.7 92/02/28
 *
 *	@(#)cons.c	8.1 (Berkeley) 6/10/93
 */

#include <lib/libsa/stand.h>

#include <machine/cpu.h>

#include "boot.h"
#include "cons.h"

#ifdef CONS_SCIF
static void scifcnprobe(struct consdev *);
static void scifcninit(struct consdev *);
static void scifcnputchar(void *, int);
static int scifcngetchar(void *);
static int scifcnscan(void *);
# include "scif.h"
# ifndef SCIFSPEED
#  define SCIFSPEED 19200
# endif
#endif

#ifdef CONS_COM
static void comcnprobe(struct consdev *);
static void comcninit(struct consdev *);
static void comcnputchar(void *, int);
static int comcngetchar(void *);
static int comcnscan(void *);
# include "ns16550.h"
# ifndef COMPORT
#  define COMPORT 0xa4000000
# endif
# ifndef COMSPEED
#  define COMSPEED 19200
# endif
#endif

static struct consdev constab[] = {
#ifdef CONS_SCIF
	{ "scif", 0, SCIFSPEED,
	    scifcnprobe, scifcninit, scifcngetchar, scifcnputchar, scifcnscan },
#endif
#ifdef CONS_COM
	{ "com", 0, COMSPEED,
	    comcnprobe, comcninit, comcngetchar, comcnputchar, comcnscan },
#endif
	{ 0 }
};

static struct consdev *cn_tab;

char *
cninit()
{
	register struct consdev *cp;

	cn_tab = NULL;
	for (cp = constab; cp->cn_probe; cp++) {
		(*cp->cn_probe)(cp);
		if (cp->cn_pri > CN_DEAD &&
		    (cn_tab == NULL || cp->cn_pri > cn_tab->cn_pri))
			cn_tab = cp;
	}
	if (cn_tab) {
		(*cn_tab->cn_init)(cn_tab);
		return cn_tab->cn_name;
	}

	return NULL;
}

int
cngetc(void)
{

	if (cn_tab)
		return (*cn_tab->cn_getc)(cn_tab->cn_dev);
	return 0;
}

void
cnputc(int c)
{

	if (cn_tab)
		(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
}

int
cnscan(void)
{

	if (cn_tab)
		return (*cn_tab->cn_scan)(cn_tab->cn_dev);
	return -1;
}

#ifdef CONS_SCIF
/*
 * sh3 scif console
 */
static void
scifcnprobe(struct consdev *cp)
{

	cp->cn_pri = CN_REMOTE;
}

static void
scifcninit(struct consdev *cp)
{

	cp->cn_dev = scif_init(cp->speed);
}

static int
scifcngetchar(void *dev)
{

	return scif_getc();
}

static void
scifcnputchar(void *dev, int c)
{

	if (c == '\n')
		scif_putc('\r');
	scif_putc(c);
}

static int
scifcnscan(void *dev)
{

	return scif_scankbd();
}
#endif /* CONS_SCIF */

#ifdef CONS_COM
/*
 * com console
 */
static void
comcnprobe(struct consdev *cp)
{

	cp->cn_pri = CN_REMOTE;
}

static void
comcninit(struct consdev *cp)
{

	cp->cn_dev = com_init(cp->address, cp->speed);
}

static int
comcngetchar(void *dev)
{

	return com_getc(dev);
}

static void
comcnputchar(void *dev, int c)
{

	if (c == '\n')
		com_putc(dev, '\r');
	com_putc(dev, c);
}

static int
comcnscan(void *dev)
{

	return com_scankbd(dev);
}
#endif /* CONS_COM */
