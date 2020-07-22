/* $NetBSD: stasc.c,v 1.2 2020/07/20 01:06:33 uwe Exp $ */
/*
 * Copyright (c) 2020 Valery Ushakov
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
 * STMicroelectronics ST40 Asynchronous Serial Controller
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: stasc.c,v 1.2 2020/07/20 01:06:33 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/cons.h>


#define STM_ASC_BASE	0xfd032000

#define ASC_BAUDRATE_OFFSET	0x00
#define ASC_TX_BUFF_OFFSET	0x04
#define ASC_RX_BUFF_OFFSET	0x08
#define ASC_CTRL_OFFSET		0x0C
#define ASC_INT_EN_OFFSET	0x10
#define ASC_INT_STA_OFFSET	0x14
#define ASC_GUARDTIME_OFFSET	0x18
#define ASC_TIMEOUT_OFFSET	0x1C
#define ASC_TX_RST_OFFSET	0x20
#define ASC_RX_RST_OFFSET	0x24
#define ASC_RETRIES_OFFSET	0x28

#define ASC_TX_BUFF	(*(volatile uint32_t *)(STM_ASC_BASE + ASC_TX_BUFF_OFFSET))
#define ASC_RX_BUFF	(*(volatile uint32_t *)(STM_ASC_BASE + ASC_RX_BUFF_OFFSET))
#define ASC_INT_EN	(*(volatile uint32_t *)(STM_ASC_BASE + ASC_INT_EN_OFFSET))
#define ASC_INT_STA	(*(volatile uint32_t *)(STM_ASC_BASE + ASC_INT_STA_OFFSET))


#define ASC_CTRL_NACK_DISABLE	0x2000
#define ASC_CTRL_BAUDMODE	0x1000
#define ASC_CTRL_CTS_EN		0x0800
#define ASC_CTRL_FIFO_EN	0x0400
#define ASC_CTRL_SC_EN		0x0200
#define ASC_CTRL_RX_EN		0x0100
#define ASC_CTRL_RUN		0x0080
#define ASC_CTRL_LOOPBACK	0x0040
#define ASC_CTRL_PARITYODD	0x0020
#define ASC_CTRL_STOPBITS_MASK	0x0018
#define     ASC_CTRL_STOPBITS_0_5	0x0000
#define     ASC_CTRL_STOPBITS_1_0	0x0008
#define     ASC_CTRL_STOPBITS_1_5	0x0010
#define     ASC_CTRL_STOPBITS_2_0	0x0018
#define ASC_CTRL_MODE_MASK	0x0007
#define     ASC_CTRL_MODE_8N		0x0001 /* 8 bit */
#define     ASC_CTRL_MODE_7P		0x0003 /* 7 bit + parity  */
#define     ASC_CTRL_MODE_9N		0x0004 /* 9 bit */
#define     ASC_CTRL_MODE_8W		0x0005 /* 8 bit + wakeup */
#define     ASC_CTRL_MODE_8P		0x0007 /* 8 bit + parity */


#define ASC_INT_EN_RHF		0x0100 /* ASC_INT_STA_RHF */
#define ASC_INT_EN_TOE		0x0080 /* ASC_INT_STA_TOE */
#define ASC_INT_EN_TNE		0x0040 /* ASC_INT_STA_TNE */
#define ASC_INT_EN_OE		0x0020 /* ASC_INT_STA_OE */
#define ASC_INT_EN_FE		0x0010 /* ASC_INT_STA_FE */
#define ASC_INT_EN_PE		0x0008 /* ASC_INT_STA_PE */
#define	ASC_INT_EN_THE		0x0004 /* ASC_INT_STA_THE */
#define ASC_INT_EN_TE		0x0002 /* ASC_INT_STA_TE */
#define ASC_INT_EN_RBF		0x0001 /* ASC_INT_STA_RBF */

#define ASC_INT_STA_NKD		0x0400 /* Tx: NACK Data */
#define ASC_INT_STA_TF		0x0200 /* Tx: Transmitter Full */
#define ASC_INT_STA_RHF		0x0100 /* Rx: Receiver FIFO Half Full */
#define ASC_INT_STA_TOE		0x0080 /* Rx: Timeout Or Empty */
#define ASC_INT_STA_TNE		0x0040 /* Rx: Timeout Or Not Empty */
#define ASC_INT_STA_OE		0x0020 /* Rx: Overrun Error */
#define ASC_INT_STA_FE		0x0010 /* Rx: Frame Error */
#define ASC_INT_STA_PE		0x0008 /* Rx: Parity Error */
#define	ASC_INT_STA_THE		0x0004 /* Tx: Transmitter FIFO Half Empty */
#define ASC_INT_STA_TE		0x0002 /* Tx: Transmitter Empty */
#define ASC_INT_STA_RBF		0x0001 /* Rx: Reciever Buffer Full */



struct stasc_softc {
	device_t sc_dev;
};


static int stasc_match(device_t, cfdata_t, void *);
static void stasc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(stasc, sizeof(struct stasc_softc),
    stasc_match, stasc_attach, NULL, NULL);


/* console */
cons_decl(stasc_)

/* assign to cn_tab after cleaning bss to get printf early for the cpu setup */
struct consdev stasc_earlycons = cons_init(stasc_);

extern struct cfdriver stasc_cd;

const struct cdevsw stasc_cdevsw = {
	.d_open = noopen,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};



static int
stasc_match(device_t parent, cfdata_t cfp, void *aux)
{

	if (strcmp(cfp->cf_name, "stasc") != 0)
		return 0;

	return 0;		/* just stub it out for now */
}


static void
stasc_attach(device_t parent, device_t self, void *aux)
{
	struct stasc_softc *sc;

	sc = device_private(self);
	sc->sc_dev = self;

	aprint_normal("\n");
}


void
stasc_cnprobe(struct consdev *cp)
{

	cp->cn_pri = CN_NORMAL;
}


void
stasc_cninit(struct consdev *cp)
{

	return;
}


int
stasc_cngetc(dev_t dev)
{
	int s = splserial();
	uint32_t status;
	int c;

	/* don't block if Rx buffer is empty */
	status = ASC_INT_STA;
	if (!ISSET(status, ASC_INT_STA_RBF)) {
		splx(s);
		return -1;
	}

	/* can read the character now */
	c = ASC_RX_BUFF;
	splx(s);
	return (unsigned char)c;
}


void
stasc_cnputc(dev_t dev, int c)
{
	int s = splserial();
	uint32_t timo, status;

	/* wait for Tx Full to become clear */
	timo = 150000;
	do {
		status = ASC_INT_STA;
	} while (ISSET(status, ASC_INT_STA_TF) && --timo);

	/* can write the character now */
	ASC_TX_BUFF = c;
	splx(s);
}


void
stasc_cnpollc(dev_t dev, int on)
{

	return;
}
