/*	$NetBSD: console.c,v 1.1 2003/08/19 10:54:59 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <dev/cons.h>

#include <machine/pcb.h>
#include <machine/io.h>

cons_decl(dte);

struct consdev dtetab = cons_init(dte);

static volatile struct dte {
	int flg;      /* Operation complete flag.  */
	int clk;      /* Clock interrupt flag.  */
	int ci;       /* Clock interrupt instruction.  */
	int t11;      /* 10 to 11 argument.  */
	int f11;      /* 10 from 11 argument.  */
	int cmd;      /* To 11 command word.  */
	int seq;      /* Operation sequence number.  */
	int opr;      /* Operational DTE #.  */
	int chr;      /* Last typed character.  */
	int mtd;      /* Monitor TTY output complete flag.  */
	int mti;      /* Monitor TTY input flag.  */
	int swr;      /* 10 switch register.  */
} *dte;

static int isinited;

void
dtecninit(struct consdev *cn)
{
	extern struct ept *ept;
	if (isinited)
		return;

	dte = (struct dte *)((int *)ept + 0444);
	cn_tab = &dtetab;
	dte->flg = dte->clk = dte->ci = 0;
	isinited++;
}

void
dtecnpollc(dev_t dev, int onoff)
{
}

void
dtecnputc(dev_t dev, int ch)
{
	dte->cmd = ch & 0177;
	dte->seq++;
	CONO(DTE,020000);
	while (dte->flg == 0)
		;
	dte->flg = 0;
}

int
dtecngetc(dev_t dev)
{
	int rch;

	while (dte->mti == 0)
		;
	rch = dte->f11 & 0177;
	dte->mti = 0;
	return rch;
}

void    
dtecnprobe(struct consdev *cn)
{
}
