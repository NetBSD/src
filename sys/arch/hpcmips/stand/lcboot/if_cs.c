/* $NetBSD: if_cs.c,v 1.2 2003/09/20 10:11:27 igy Exp $ */

/*
 * Copyright (c) 2003 Naoto Shimazaki.
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
 * THIS SOFTWARE IS PROVIDED BY NAOTO SHIMAZAKI AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE NAOTO OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cs.c,v 1.2 2003/09/20 10:11:27 igy Exp $");

#include <sys/param.h>
#include <netinet/in.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/netif.h>

#include <dev/ic/cs89x0reg.h>

#include "extern.h"

static int	cs_match(struct netif *, void *);
static int	cs_probe(struct netif *, void *);
static void	cs_init(struct iodesc *, void *);
static int	cs_get(struct iodesc *, void *, size_t, time_t);
static int	cs_put(struct iodesc *, void *, size_t);
static void	cs_end(struct netif *);

static struct netif_stats	cs_stats;

static struct netif_dif	cs_if = {
	.dif_unit	= 0,
	.dif_nsel	= 1,
	.dif_stats	= &cs_stats,
	.dif_private	= NULL,
	.dif_used	= 0,
};

struct netif_driver	cs_driver = {
	.netif_bname	= "cs",
	.netif_match	= cs_match,
	.netif_probe	= cs_probe,
	.netif_init	= cs_init,
	.netif_get	= cs_get,
	.netif_put	= cs_put,
	.netif_end	= cs_end,
	.netif_ifs	= &cs_if,
	.netif_nifs	= 1,
};

#define CS_IO_BASE	0x14010300U

#define CS_READ_1(off)		REGREAD_1(CS_IO_BASE, (off))
#define CS_READ_2(off)		REGREAD_2(CS_IO_BASE, (off))
#define CS_WRITE_1(off, val)	REGWRITE_1(CS_IO_BASE, (off), (val))
#define CS_WRITE_2(off, val)	REGWRITE_2(CS_IO_BASE, (off), (val))
#define CS_READ_PACKET_PAGE(off)					\
		(REGWRITE_2(CS_IO_BASE, PORT_PKTPG_PTR, (off)),		\
		 REGREAD_2(CS_IO_BASE, PORT_PKTPG_DATA))
#define CS_WRITE_PACKET_PAGE(off, val)					\
		(REGWRITE_2(CS_IO_BASE, PORT_PKTPG_PTR, (off)),		\
		 REGWRITE_2(CS_IO_BASE, PORT_PKTPG_DATA, (val)))

static inline void
delay(int n)
{
	int	i = 33 * n;

	while (--i > 0)
		;
}

time_t
getsecs(void)
{
	return REGREAD_4(VRETIMEL, 0) >> 15;
}

static int
cs_match(struct netif *nif, void *machdep_hint)
{
	return 1;
}

static int
cs_probe(struct netif *nif, void *machdep_hint)
{
	return 0;
}

static void
cs_get_eeprom(int offset, u_int16_t *result)
{
	int	timeo;

	for (timeo = MAXLOOP; timeo > 0; timeo--) {
		if (!(CS_READ_PACKET_PAGE(PKTPG_SELF_ST)
		      & SELF_ST_SI_BUSY))
			break;
	}
	if (timeo == 0)
		goto eeprom_error;

	CS_WRITE_PACKET_PAGE(PKTPG_EEPROM_CMD, offset | EEPROM_CMD_READ);

	for (timeo = MAXLOOP; timeo > 0; timeo--) {
		if (!(CS_READ_PACKET_PAGE(PKTPG_SELF_ST)
		      & SELF_ST_SI_BUSY))
			break;
	}
	if (timeo == 0)
		goto eeprom_error;

	*result = CS_READ_PACKET_PAGE(PKTPG_EEPROM_DATA);

	return;

eeprom_error:
	panic("cannot read mac addr");
}

static void
cs_init(struct iodesc *desc, void *machdep_hint)
{
	int		i;
	u_int16_t	*myea;

	/* Issue a software reset command to the chip */
	CS_WRITE_PACKET_PAGE(PKTPG_SELF_CTL, SELF_CTL_RESET);

	/* We cannot touch the chip until calibration is done */
	delay(10000);

	/*
	 * Transition -SBHE H->L L->H is needed between reset and
	 * the first access to the chip's register.
	 */
	CS_READ_1(PORT_PKTPG_PTR + 0);
	CS_READ_1(PORT_PKTPG_PTR + 1);
	CS_READ_1(PORT_PKTPG_PTR + 0);
	CS_READ_1(PORT_PKTPG_PTR + 1);

	/* wait for INIT_DONE */
	for (i = 10000; i > 0; i--) {
		u_int16_t	s;

		s = CS_READ_PACKET_PAGE(PKTPG_SELF_ST);
		if ((s & SELF_ST_INIT_DONE) && !(s & SELF_ST_SI_BUSY))
			break;
	}
	if (i == 0)
		panic("cannot reset netif");

	myea = (u_int16_t *) desc->myea;

	cs_get_eeprom(EEPROM_IND_ADDR_H, &myea[0]);
	cs_get_eeprom(EEPROM_IND_ADDR_M, &myea[1]);
	cs_get_eeprom(EEPROM_IND_ADDR_L, &myea[2]);

	for (i = 0; i < 3; i++)
		CS_WRITE_PACKET_PAGE(PKTPG_IND_ADDR + (i << 1), myea[i]);

	/*
	 * Accepting frames:
	 *	RX_CTL_RX_OK_A:	correct crc, and valid length
	 *	RX_CTL_IND_A:	dest addr maches individual address
	 *	RX_CTL_BCAST_A:	dest addr maches broadcast address
	 */
	CS_WRITE_PACKET_PAGE(PKTPG_RX_CTL,
			     RX_CTL_RX_OK_A | RX_CTL_IND_A | RX_CTL_BCAST_A);
	CS_WRITE_PACKET_PAGE(PKTPG_LINE_CTL, LINE_CTL_RX_ON | LINE_CTL_TX_ON);
}

static int
cs_get(struct iodesc *desc, void *pkt, size_t len, time_t timeout)
{
	time_t		t;
	int		rlen;
	int		i;
	u_int16_t	*p;

	t = getsecs();
	rlen = 0;
	while (getsecs() - t < timeout && rlen == 0) {
		if (!(CS_READ_PACKET_PAGE(PKTPG_RX_EVENT) & RX_EVENT_RX_OK))
			continue;

		/* drop status */
		CS_READ_2(PORT_RXTX_DATA);

		/* get frame length */
		rlen = CS_READ_2(PORT_RXTX_DATA);

		if (rlen > len) {
			CS_WRITE_PACKET_PAGE(PKTPG_RX_CFG, RX_CFG_SKIP);
			rlen = 0;
			continue;
		}

		p = pkt;
		for (i = rlen >> 1; i > 0; i--)
			*p++ = CS_READ_2(PORT_RXTX_DATA);
		if (rlen & 1)
			*((u_int8_t *) p + 1) = CS_READ_1(PORT_RXTX_DATA);

		/* exit while loop */
	}

	return rlen;
}

static int
cs_put(struct iodesc *desc, void *pkt, size_t len)
{
	int		timeo;
	int		i;
	u_int16_t	*p;

	CS_WRITE_2(PORT_TX_CMD, TX_CMD_START_ALL);
	CS_WRITE_2(PORT_TX_LENGTH, len);

	for (timeo = 1000000; timeo > 0; timeo--) {
		if (CS_READ_PACKET_PAGE(PKTPG_BUS_ST) & BUS_ST_RDY4TXNOW)
			break;
	}
	if (timeo == 0)
		panic("cs: cannot send frame");

	p = pkt;
	i = (len + 1) >> 1;
	while (i > 0) {
		CS_WRITE_2(PORT_RXTX_DATA, *p++);
		i--;
	}

	return len;
}

static void
cs_end(struct netif *nif)
{
	CS_WRITE_PACKET_PAGE(PKTPG_LINE_CTL, 0);
}
