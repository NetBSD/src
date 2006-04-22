/*	$NetBSD: ssc.c,v 1.2.6.2 2006/04/22 11:37:36 simonb Exp $	*/

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
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/device.h>

#include <machine/md_var.h>
#include <machine/ssc.h>

#include <dev/cons.h>


#define SSC_GETCHAR			21
#define SSC_PUTCHAR			31

#define	SSC_POLL_HZ	50

void sscconsattach(struct device *, struct device *, void *); 

static void ssccnprobe(struct consdev *);
static void ssccninit(struct consdev *);
static void ssccnputc(dev_t, int);
static int  ssccngetc(dev_t);

struct consdev constab[] = {
	{ ssccnprobe, ssccninit, ssccngetc, ssccnputc, nullcnpollc, 
	  NULL, NULL, NULL, NODEV, CN_NORMAL },
	{ NULL }
};


u_int64_t
ssc(u_int64_t in0, u_int64_t in1, u_int64_t in2, u_int64_t in3, int which)
{
	register u_int64_t ret0 __asm("r8");

	__asm __volatile("mov r15=%1\n\t"
			 "break 0x80001"
			 : "=r"(ret0)
			 : "r"(which), "r"(in0), "r"(in1), "r"(in2), "r"(in3));
	return ret0;
}

void sscconsattach(struct device *parent, struct device *self, void *aux) 
{ 
}

static void
ssccnprobe(struct consdev *cp)
{
	cp->cn_pri = CN_INTERNAL;
}

static void
ssccninit(struct consdev *cp)
{
}

/* void */
/* ssccnattach(void *arg) */
/* { */
/* 	static struct consdev ssccons = { */
/* 		ssccnprobe, ssccninit, ssccngetc, ssccnputc, nullcnpollc, */
/* 		NULL, NULL, NULL, NODEV, CN_NORMAL */
/* 	}; */

/* 	cn_tab = &ssccons; */
/* } */

static void
ssccnputc(dev_t dev, int c)
{
	ssc(c, 0, 0, 0, SSC_PUTCHAR);
}

static int
ssccngetc(dev_t dev)
{
	int c;
	do {
		c = ssc(0, 0, 0, 0, SSC_GETCHAR);
	} while (c == 0);

	return c;
}

/* XXX: integrate the rest of the ssc.c stuff from FreeBSD to plug into wsdisplay */

