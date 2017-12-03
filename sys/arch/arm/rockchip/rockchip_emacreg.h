/* $NetBSD: rockchip_emacreg.h,v 1.1.18.2 2017/12/03 11:35:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ROCKCHIP_EMACREG_H
#define _ROCKCHIP_EMACREG_H

#define EMAC_ID_REG		0x0000
#define EMAC_STAT_REG		0x0004
#define EMAC_ENABLE_REG		0x0008
#define EMAC_CONTROL_REG	0x000c
#define EMAC_POLLRATE_REG	0x0010
#define EMAC_RXERR_REG		0x0014
#define EMAC_MISS_REG		0x0018
#define EMAC_TXRINGPTR_REG	0x001c
#define EMAC_RXRINGPTR_REG	0x0020
#define EMAC_ADDRL_REG		0x0024
#define EMAC_ADDRH_REG		0x0028
#define EMAC_LAFL_REG		0x002c
#define EMAC_LAFH_REG		0x0030
#define EMAC_MDIO_DATA_REG	0x0034
#define EMAC_TXRINGPTR_READ_REG	0x0038
#define EMAC_RXRINGPTR_READ_REG	0x003c

#define EMAC_ID_REVISION	__BITS(23,16)
#define EMAC_ID_NOTID		__BITS(13,8)
#define EMAC_ID_ID		__BITS(5,0)

#define EMAC_STAT_TXPL		__BIT(31)
#define EMAC_STAT_MDIO		__BIT(12)
#define EMAC_STAT_RXFL		__BIT(10)
#define EMAC_STAT_RXFR		__BIT(9)
#define EMAC_STAT_RXCR		__BIT(8)
#define EMAC_STAT_MSER		__BIT(4)
#define EMAC_STAT_TXCH		__BIT(3)
#define EMAC_STAT_ERR		__BIT(2)
#define EMAC_STAT_RXINT		__BIT(1)
#define EMAC_STAT_TXINT		__BIT(0)

#define EMAC_ENABLE_TXPL	__BIT(31)
#define EMAC_ENABLE_MDIO	__BIT(12)
#define EMAC_ENABLE_RXFL	__BIT(10)
#define EMAC_ENABLE_RXFR	__BIT(9)
#define EMAC_ENABLE_RXCR	__BIT(8)
#define EMAC_ENABLE_MSER	__BIT(4)
#define EMAC_ENABLE_TXCH	__BIT(3)
#define EMAC_ENABLE_ERR		__BIT(2)
#define EMAC_ENABLE_RXINT	__BIT(1)
#define EMAC_ENABLE_TXINT	__BIT(0)

#define EMAC_CONTROL_RXBDTLEN	__BITS(31,24)
#define EMAC_CONTROL_TXBDTLEN	__BITS(23,16)
#define EMAC_CONTROL_DISAD	__BIT(15)
#define EMAC_CONTROL_DISRT	__BIT(14)
#define EMAC_CONTROL_TEST	__BIT(13)
#define EMAC_CONTROL_DIS2P	__BIT(12)
#define EMAC_CONTROL_PROM	__BIT(11)
#define EMAC_CONTROL_ENFL	__BIT(10)
#define EMAC_CONTROL_DISBC	__BIT(8)
#define EMAC_CONTROL_RXRN	__BIT(4)
#define EMAC_CONTROL_TXRN	__BIT(3)
#define EMAC_CONTROL_EN		__BIT(0)

#define EMAC_POLLRATE_POLLRATE	__BITS(14,0)

#define EMAC_RXERR_RXOFLOW	__BITS(23,16)
#define EMAC_RXERR_RXFRAM	__BITS(15,8)
#define EMAC_RXERR_RXCRC	__BITS(7,0)

#define EMAC_MISS_MISSCNTR	__BITS(7,0)

#define EMAC_MDIO_DATA_SFD	__BITS(31,30)
#define EMAC_MDIO_DATA_SFD_SFD	1
#define EMAC_MDIO_DATA_OP	__BITS(29,28)
#define EMAC_MDIO_DATA_OP_WRITE	1
#define EMAC_MDIO_DATA_OP_READ	2
#define EMAC_MDIO_DATA_PHYADDR	__BITS(27,23)
#define EMAC_MDIO_DATA_PHYREG	__BITS(22,18)
#define EMAC_MDIO_DATA_TA	__BITS(17,16)
#define EMAC_MDIO_DATA_TA_TA	2
#define EMAC_MDIO_DATA_DATA	__BITS(15,0)

struct rkemac_txdesc {
	uint32_t	tx_info;
/* CPU bits */
#define EMAC_TXDESC_OWN		__BIT(31)
#define EMAC_TXDESC_ADCR	__BIT(18)
#define EMAC_TXDESC_LAST	__BIT(17)
#define EMAC_TXDESC_FIRST	__BIT(16)
#define EMAC_TXDESC_TXLEN	__BITS(10,0)
/* VMAC bits */
#define EMAC_TXDESC_UFLO	__BIT(29)
#define EMAC_TXDESC_LATECOL	__BIT(28)
#define EMAC_TXDESC_RTRY	__BITS(27,24)
#define EMAC_TXDESC_DRP		__BIT(23)
#define EMAC_TXDESC_DEFR	__BIT(22)
#define EMAC_TXDESC_CRLS	__BIT(21)
	uint32_t	tx_ptr;
};

struct rkemac_rxdesc {
	uint32_t	rx_info;
/* CPU bits */
#define EMAC_RXDESC_OWN		__BIT(31)
#define EMAC_RXDESC_LAST	__BIT(17)
#define EMAC_RXDESC_FIRST	__BIT(16)
#define EMAC_RXDESC_RXLEN	__BITS(10,0)
/* VMAC bits */
#define EMAC_RXDESC_BUFF	__BIT(30)
	uint32_t	rx_ptr;
};



#endif /* !_ROCKCHIP_EMACREG_H */
