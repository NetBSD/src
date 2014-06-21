/*	$NetBSD: dcm.c,v 1.9 2014/06/21 02:02:40 tsutsui Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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
 *	@(#)dcm.c	8.1 (Berkeley) 6/10/93
 */

#ifdef DCMCONSOLE
#include <sys/param.h>
#include <dev/cons.h>

#include <hp300/dev/dcmreg.h>

#include <hp300/stand/common/consdefs.h>
#include <hp300/stand/common/samachdep.h>
#include <hp300/stand/common/device.h>

struct dcmdevice *dcmcnaddr = NULL;

#define	DCMCONUNIT	1	/* XXX */

void
dcmprobe(struct consdev *cp)
{
	struct hp_hw *hw;
	struct dcmdevice *dcm;

	for (hw = sc_table; hw < &sc_table[MAXCTLRS]; hw++)
		if (HW_ISDEV(hw, D_COMMDCM) && !badaddr((void *)hw->hw_kva))
			break;
	if (!HW_ISDEV(hw, D_COMMDCM)) {
		cp->cn_pri = CN_DEAD;
		return;
	}
	dcmcnaddr = (struct dcmdevice *) hw->hw_kva;

#ifdef FORCEDCMCONSOLE
	cp->cn_pri = CN_REMOTE;
#else
	dcm = dcmcnaddr;
	switch (dcm->dcm_rsid) {
	case DCMID:
		cp->cn_pri = CN_NORMAL;
		break;
	case DCMID|DCMCON:
		cp->cn_pri = CN_REMOTE;
		break;
	default:
		cp->cn_pri = CN_DEAD;
		break;
	}

	curcons_scode = hw->hw_sc;
#endif
}

void
dcminit(struct consdev *cp)
{
	struct dcmdevice *dcm = dcmcnaddr;
	int port = DCMCONUNIT;

	dcm->dcm_ic = IC_ID;
	while (dcm->dcm_thead[port].ptr != dcm->dcm_ttail[port].ptr)
		;
	dcm->dcm_data[port].dcm_baud = BR_9600;
	dcm->dcm_data[port].dcm_conf = LC_8BITS | LC_1STOP;
	SEM_LOCK(dcm);
	dcm->dcm_cmdtab[port].dcm_data |= CT_CON;
	dcm->dcm_cr |= (1 << port);
	SEM_UNLOCK(dcm);
	DELAY(15000);
}

/* ARGSUSED */
#ifndef SMALL
int
dcmgetchar(dev_t dev)
{
	struct dcmdevice *dcm = dcmcnaddr;
	struct dcmrfifo *fifo;
	struct dcmpreg *pp;
	unsigned int head;
	int c, port;

	port = DCMCONUNIT;
	pp = dcm_preg(dcm, port);
	head = pp->r_head & RX_MASK;
	if (head == (pp->r_tail & RX_MASK))
		return 0;
	fifo = &dcm->dcm_rfifos[3-port][head>>1];
	c = fifo->data_char;
	(void)fifo->data_stat;
	pp->r_head = (head + 2) & RX_MASK;
	SEM_LOCK(dcm);
	(void)dcm->dcm_iir;
	SEM_UNLOCK(dcm);
	return c;
}
#else
int
dcmgetchar(dev_t dev)
{

	return 0;
}
#endif

/* ARGSUSED */
void
dcmputchar(dev_t dev, int c)
{
	struct dcmdevice *dcm = dcmcnaddr;
	struct dcmpreg *pp;
	int timo;
	unsigned int tail;
	int port;

	port = DCMCONUNIT;
	pp = dcm_preg(dcm, port);
	tail = pp->t_tail & TX_MASK;
	timo = 50000;
	while (tail != (pp->t_head & TX_MASK) && --timo)
		;
	dcm->dcm_tfifos[3-port][tail].data_char = c;
	pp->t_tail = tail = (tail + 1) & TX_MASK;
	SEM_LOCK(dcm);
	dcm->dcm_cmdtab[port].dcm_data |= CT_TX;
	dcm->dcm_cr |= (1 << port);
	SEM_UNLOCK(dcm);
	timo = 1000000;
	while (tail != (pp->t_head & TX_MASK) && --timo)
		;
	SEM_LOCK(dcm);
	(void)dcm->dcm_iir;
	SEM_UNLOCK(dcm);
}
#endif
