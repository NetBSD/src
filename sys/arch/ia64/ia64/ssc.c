/*	$NetBSD: ssc.c,v 1.2.78.1 2009/08/19 18:46:21 yamt Exp $	*/

/*-
 * Copyright (c) 2000 Doug Rabson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD$
 */

#include <sys/param.h>

#include <machine/ssc.h>

#include <dev/cons.h>


#define	SSC_POLL_HZ	50

void sscconsattach(struct device *, struct device *, void *);

void ssccnprobe(struct consdev *);
void ssccninit(struct consdev *);
void ssccnputc(dev_t, int);
int ssccngetc(dev_t);
void ssccnpollc(dev_t, int);


uint64_t
ssc(uint64_t in0, uint64_t in1, uint64_t in2, uint64_t in3, int which)
{
	register uint64_t ret0 __asm("r8");

	__asm __volatile("mov r15=%1\n\t"
			 "break 0x80001"
			 : "=r"(ret0)
			 : "r"(which), "r"(in0), "r"(in1), "r"(in2), "r"(in3));
	return ret0;
}


void
sscconsattach(struct device *parent, struct device *self, void *aux)
{
	/* not yet */
}

void
ssccnprobe(struct consdev *cp)
{

	cp->cn_dev = ~NODEV;		/* XXXX: And already exists */
	cp->cn_pri = CN_INTERNAL;
}

void
ssccninit(struct consdev *cp)
{
	/* nothing */
}

void
ssccnputc(dev_t dev, int c)
{

	ssc(c, 0, 0, 0, SSC_PUTCHAR);
}

int
ssccngetc(dev_t dev)
{
	int c;

	do {
		c = ssc(0, 0, 0, 0, SSC_GETCHAR);
	} while (c == 0);

	return c;
}

void
ssccnpollc(dev_t dev, int on)
{
	/* nothing */
}

/* XXX: integrate the rest of the ssc.c stuff from FreeBSD to plug into wsdisplay */
