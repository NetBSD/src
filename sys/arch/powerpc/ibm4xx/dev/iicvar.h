/*	$NetBSD: iicvar.h,v 1.1 2003/09/23 14:56:08 shige Exp $	*/

/*
 * Copyright 2003 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IBM4XX_IICVAR_H_
#define	_IBM4XX_IICVAR_H_

#include <machine/bus.h>

/*
 * Software state per device.
 */

struct iicbus_attach_args {
	const char		*iicb_name;
	u_long			iicb_addr;
	int			iicb_irq;
	bus_space_tag_t		iicb_bt;	/* Bus space tag */
	bus_dma_tag_t		iicb_dmat;	/* DMA tag */
	struct iic_softc	*adap;		/* IIC Adapter */
};

struct iic_softc {
	struct device		sc_dev;		/* generic device information */
	bus_space_tag_t		sc_st;		/* bus space tag */
	bus_space_handle_t	sc_sh;		/* bus space handle */
	bus_dma_tag_t		sc_dmat;	/* bus DMA tag */
};

/*
 * definitions for IIC Transfer
 */
#define IIC_XFER_WRITE		(0)
#define IIC_XFER_READ		(1)

#define IIC_XFER_NRMST		(0)	/* Normal start */
#define IIC_XFER_RPTST		(1)	/* Repeated start */

/*
 * definitions for IIC Addressing Mode
 */
#define	IIC_ADM_7BIT		(0)	/* Addressing Mode 7-bit */
#define	IIC_ADM_10BIT		(1)	/* Addressing Mode 10-bit */

/*
 * definitions for IIC Error code
 */
#define IIC_ERR_NOBUSFREE	(-1)

/*
 * functions
 */
int iic_read(struct iic_softc *sc, u_char saddr,
	u_char *addr, int alen, u_char *data, int dlen);
int iic_write(struct iic_softc *sc, u_char saddr,
	u_char *addr, int alen, u_char *data, int dlen);

#endif	/* _IBM4XX_IICVAR_H_ */
