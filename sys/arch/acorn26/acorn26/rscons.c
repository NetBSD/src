/* $NetBSD: rscons.c,v 1.2.6.2 2002/06/23 17:33:36 jdolecek Exp $ */
/*-
 * Copyright (c) 1997, 1998 Ben Harris
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
 *    derived from this software without specific prior written permission.
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
/*
 * rscons.c - Really basic routines for the Arc serial port
 */

#include <sys/param.h>
#include <sys/conf.h>

#include <dev/cons.h>
#include <dev/ic/r6551reg.h>

#define RS_BASE 0x033B0000

cons_decl(rs);

void rscnprobe(cp)
	struct consdev *cp;
{

	/* Be optimistic! */
	cp->cn_pri = CN_NORMAL;
	cp->cn_dev = makedev(13, 0);
}

void rscninit(cp)
	struct consdev *cp;
{
	volatile char *rs_base = (volatile char *)RS_BASE;

	/* Reset 6551 */
	rs_base[R6551_RESET<<2] = 1;
	/* 9600, 8N1 */
	rs_base[R6551_CONTROL<<2] = R6551_CTL_SBR_9600 |
		R6551_CTL_WL_8 | R6551_CTL_RCS_INT;
	/* Enable transmission, disable interrupts, set DTR. */
	rs_base[R6551_COMMAND<<2] = R6551_CMD_DTR | R6551_CMD_IRD |
		R6551_CMD_TIC_NOIRQ;
	/* The 6551 is now sane.  Well, as sane as a 6551 can be.  */
}

int rscngetc(dev)
	dev_t dev;
{
	volatile char *rs_base = (volatile char *)RS_BASE;
	int c;

	/* Wait for reception */
	while ((rs_base[R6551_STATUS<<2] & R6551_STAT_RDRF) == 0);
	c = rs_base[R6551_RX_DATA<<2];
	if (c == '\r')
		c = '\n';
	return c;
}

void rscnputc(dev, c)
	dev_t dev;
	int c;
{
	volatile char *rs_base = (volatile char *)RS_BASE;

	while (!(rs_base[R6551_STATUS<<2] & R6551_STAT_TDRE));
	/* And it's now ready to transmit.  */
        rs_base[R6551_TX_DATA<<2] = c;
	/* Wait for it to finish (me?  paranoid?) */
	while (!(rs_base[R6551_STATUS<<2] & R6551_STAT_TDRE));
}	

void rscnpollc(dev, on)
	dev_t dev;
	int on;
{
}
