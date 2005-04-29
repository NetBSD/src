/*
 * Copyright (c) 2002, 2005  Genetec corp.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corp. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EVBARM_G42XXEB_REG_H
#define _EVBARM_G42XXEB_REG_H

#include <arm/xscale/pxa2x0reg.h>

/* g42xxeb on-board IOs */
#define G42XXEB_PLDREG_BASE	PXA2X0_CS3_START /* Phisical address */
#define G42XXEB_PLDREG_SIZE	0x00000100

#define G42XXEB_AX88796_PBASE (PXA2X0_CS3_START+0x02000000)


/*
 * Logical mapping for onboard/integrated peripherals
 * that are used while bootstrapping.
 */
#define G42XXEB_IO_AREA_VBASE  0xfd000000
#define G42XXEB_PLDREG_VBASE	0xfd000000
#define G42XXEB_INTCTL_VBASE	0xfd100000
#define G42XXEB_CLKMAN_VBASE	0xfd200000
#define G42XXEB_GPIO_VBASE	0xfd300000
#define G42XXEB_FFUART_VBASE	0xfd400000
#define G42XXEB_BTUART_VBASE	0xfd500000

/*
 * Onboard register address
 * (offset from G42XXEB_OBIO_PBASE)
 */
#define G42XXEB_INTSTS1	0x0a
#define G42XXEB_INTSTS2	0x0c
#define G42XXEB_INTCNTL  	0x0e
#define G42XXEB_INTCNTH  	0x10
#define G42XXEB_INTMASK	0x14

#define  G42XXEB_INT_MMCSD     0
#define  G42XXEB_INT_SDIO      1
#define  G42XXEB_INT_EXT0      2
#define  G42XXEB_INT_EXT1      3
#define  G42XXEB_INT_USB       4
#define  G42XXEB_INT_ETH       5
#define  G42XXEB_INT_CODEC     6
#define  G42XXEB_INT_EXT2      7
#define  G42XXEB_INT_KEY       8
#define  G42XXEB_INT_EXT3      9

#define  G42XXEB_N_INTS 	10

/* interrupt type */
#define G42XXEB_INT_LEVEL_LOW   	0
#define G42XXEB_INT_LEVEL_HIGH  	1
#define G42XXEB_INT_EDGE_FALLING	4
#define G42XXEB_INT_EDGE_RISING	5
#define G42XXEB_INT_EDGE_BOTH  	6


#define G42XXEB_DIPSW 		0x16
#define G42XXEB_LED   		0x18

#define G42XXEB_RST        	0x1a
#define  RST_ASIX88796     	(1<<0)
#define  RST_EXT(n) 		(1<<((n)+1))
#define G42XXEB_EXTCTRL	0x1c
#define G42XXEB_OPTBRDID       0x20
#define G42XXEB_PLDVER         0x22


#define G42XXEB_LCDCTL         0x28
#define  LCDCTL_BL_ON	(1<<7)
#define  LCDCTL_DPSH	(1<<1)
#define  LCDCTL_DPSV	(1<<0)
#define  LCDCTL_BL_PWM_SHIFT  8		/* Backlight blightness */
#define  LCDCTL_BL_PWN  (0xff<<LCDCTL_BL_PWM_SHIFT)

#define G42XXEB_KEYSCAN	0x2a
#define  KEYSCAN_SCAN_OUT   	0x1f00	
#define  KEYSCAN_SENSE_IN       0x0f 
#define G42XXEB_WP         	0x2c // SD/MMC write protect status

#define ioreg_read(a)  (*(volatile unsigned *)(a))
#define ioreg_write(a,v)  (*(volatile unsigned *)(a)=(v))

#define ioreg16_read(a)  (*(volatile uint16_t *)(a))
#define ioreg16_write(a,v)  (*(volatile uint16_t *)(a)=(v))

#define ioreg8_read(a)  (*(volatile uint8_t *)(a))
#define ioreg8_write(a,v)  (*(volatile uint8_t *)(a)=(v))

#define pldreg16_read(off)	ioreg16_read(G42XXEB_PLDREG_VBASE+(off))
#define pldreg16_write(off,v)	ioreg16_write(G42XXEB_PLDREG_VBASE+(off),v)
#define pldreg8_read(off)	ioreg8_read(G42XXEB_PLDREG_VBASE+(off))
#define pldreg8_write(off,v)	ioreg8_write(G42XXEB_PLDREG_VBASE+(off),v)

#endif /* _EVBARM_G42XXEB_REG_H */
