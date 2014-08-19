/* $NetBSD: omapl1x_reg.h,v 1.1.10.2 2014/08/20 00:02:47 tls Exp $ */
/*
 * Copyright (c) 2013 Linu Cherian
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_OMAP_OMAPL1X_REG_H_
#define _ARM_OMAP_OMAPL1X_REG_H_

#include "opt_omapl1x.h"

#ifndef MEMSIZE
#error Specify the amount of SDRAM in megabytes with the MEMSIZE option.
#endif
#define MEMSIZE_BYTES (MEMSIZE * 1024 * 1024)

/* OMAPL138 Memory Map */
#define OMAPL1X_TIMER_SIZE	0x1000
#define OMAPL1X_UART_SIZE	0x1000
#define OMAPL1X_AINTC_SIZE	0x2000
#define OMAPL1X_EMAC_SIZE	0x3000

#define OMAPL1X_SYSCFG0_ADDR	0x01c14000
#define OMAPL1X_SYSCFG0_SIZE	0x190

#define OMAPL1X_WDT_ADDR	0x01C21000
#define OMAPL1X_WDT_SIZE	0x80

/* Timer */

#define PID12			0x00
#define TIM12			0x10
#define TIM34			0x14
#define PRD12			0x18
#define PRD34			0x1c
#define TCR			0x20
#define TGCR			0x24
#define INTCTLSTAT		0x44

#define WDTCR			0x28


/* SYSCFG */

#define KICK0R			0x038
#define KICK1R			0x03C
#define CFGCHIP2		0x184


/* AINTC */

#define AINTC_REVID		0x0000
#define AINTC_CR		0x0004
#define AINTC_GER		0x0010
#define AINTC_GNLR		0x001C
#define AINTC_SISR		0x0020
#define AINTC_SICR		0x0024
#define AINTC_EISR		0x0028
#define AINTC_EICR		0x002C
#define AINTC_HIEISR		0x0034
#define AINTC_HIEICR		0x0038
#define AINTC_VBR		0x0050
#define AINTC_VSR		0x0054
#define AINTC_VNR		0x0058
#define AINTC_GPIR		0x0080
#define AINTC_GPVR		0x0084
#define AINTC_SRSR1		0x0200
#define AINTC_SRSR2		0x0204
#define AINTC_SRSR3		0x0208
#define AINTC_SRSR4		0x020C
#define AINTC_SECR1		0x0280
#define AINTC_SECR2		0x0284
#define AINTC_SECR3		0x0288
#define AINTC_SECR4		0x028C
#define AINTC_ESR1		0x0300
#define AINTC_ESR2		0x0304
#define AINTC_ESR3		0x0308
#define AINTC_ESR4		0x030C
#define AINTC_ECR1		0x0380
#define AINTC_ECR2		0x0384
#define AINTC_ECR3		0x0388
#define AINTC_ECR4		0x038c

/*
 * FFFE E400h CMR0-CMR25 Channel
 * Map Registers 0-25 Section
 * 12.4.32 FFFE E464h
 */

#define AINTC_CMR0		0x0400

#define AINTC_HIPIR1		0x0900
#define AINTC_HIPIR2		0x0904
#define AINTC_HINLR1		0x1100
#define AINTC_HINLR2		0x1104
#define AINTC_HIER		0x1500
#define AINTC_HIPVR1		0x1600
#define AINTC_HIPVR2		0x1604


/* PSC */

#define PTSTAT		0x128
#define PTCMD		0x120
#define MDSTAT		0x800
#define MDCTL		0xA00


/* EMAC */

#define MAC_OFFSET		0x1000
#define MACTXCONTROL		(MAC_OFFSET + 0x4)
#define MACTXTEARDOWN		(MAC_OFFSET + 0x8)
#define MACRXCONTROL		(MAC_OFFSET + 0x14)
#define MACRXTEARDOWN		(MAC_OFFSET + 0x18)
#define MACTXINTMASKSET		(MAC_OFFSET + 0x88)
#define MACTXINTMASKCLEAR	(MAC_OFFSET + 0x8C)
#define MACINVECTOR		(MAC_OFFSET + 0x90)
#define MACEOIVECTOR		(MAC_OFFSET + 0x94)
#define MACRXINTMASKSET		(MAC_OFFSET + 0xA8)
#define MACRXINTMASKCLEAR	(MAC_OFFSET + 0xAC)
#define MACINTMASKSET		(MAC_OFFSET + 0xB8)
#define MACRXMBPEN		(MAC_OFFSET + 0x100)
#define MACRXUNICASTSET		(MAC_OFFSET + 0x104)
#define MACRXUNICASTCLEAR	(MAC_OFFSET + 0x108)
#define MACRXMAXLEN		(MAC_OFFSET + 0x10C)
#define MACRXBUFOFFSET		(MAC_OFFSET + 0x110)
#define MACCONTROL		(MAC_OFFSET + 0x160)
#define MACSOFTRESET		(MAC_OFFSET + 0x174)
#define MACSRCADDRLO		(MAC_OFFSET + 0x1D0)
#define MACSRCADDRHI		(MAC_OFFSET + 0x1D4)
#define MACHASH1		(MAC_OFFSET + 0x1D8)
#define MACHASH2		(MAC_OFFSET + 0x1DC)
#define MACADDRLO		(MAC_OFFSET + 0x500)
#define MACADDRHI		(MAC_OFFSET + 0x504)
#define MACINDEX		(MAC_OFFSET + 0x508)

#define MAC_TX_HDP(p)		(MAC_OFFSET + 0x600 + ((p) * 0x04))
#define MAC_TX_CP(p)		(MAC_OFFSET + 0x640 + ((p) * 0x04))

#define MAC_RX_HDP(p)		(MAC_OFFSET + 0x620 + ((p) * 0x04))
#define MAC_RX_CP(p)		(MAC_OFFSET + 0x660 + ((p) * 0x04))

#define MAC_CR_OFFSET			0x0
#define MAC_CR_SOFT_RESET		(MAC_CR_OFFSET + 0x04)
#define MAC_CR_INT_CONTROL		(MAC_CR_OFFSET + 0x0c)
#define MAC_CR_C_RX_THRESH_EN(p)	(MAC_CR_OFFSET + (0x10 * (p)) + 0x10)
#define MAC_CR_C_RX_EN(p)		(MAC_CR_OFFSET + (0x10 * (p)) + 0x14)
#define MAC_CR_C_TX_EN(p)		(MAC_CR_OFFSET + (0x10 * (p)) + 0x18)
#define MAC_CR_C_MISC_EN(p)		(MAC_CR_OFFSET + (0x10 * (p)) + 0x1C)
#define MAC_CR_C_RX_THRESH_STAT(p)	(MAC_CR_OFFSET + (0x10 * (p)) + 0x40)
#define MAC_CR_C_RX_STAT(p)		(MAC_CR_OFFSET + (0x10 * (p)) + 0x44)
#define MAC_CR_C_TX_STAT(p)		(MAC_CR_OFFSET + (0x10 * (p)) + 0x48)
#define MAC_CR_C_MISC_STAT(p)		(MAC_CR_OFFSET + (0x10 * (p)) + 0x4C)

#define MAC_MDIO_OFFSET			0x2000
#define MACMDIOCONTROL			(MAC_MDIO_OFFSET + 0x4)
#define MACMDIOUSERACCESS0		(MAC_MDIO_OFFSET + 0x80)

#define EMAC_CPPI_RAM_BASE		0x01E20000
#define EMAC_CPPI_RAM_SIZE		0x2000

#endif /* _ARM_OMAP_OMAPL1X_REG_H_ */
