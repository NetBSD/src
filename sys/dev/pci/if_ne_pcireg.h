/*	$NetBSD: if_ne_pcireg.h,v 1.1 1998/10/27 22:30:56 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Register description for NE2000-compatible PCI Ethernet interfaces.
 */

/*
 * Registers on RealTek 8029 NE2000-compatible network interfaces.
 *
 * Data sheets for this chip can be found at:
 *
 *	http://www.realtek.com.tw
 */

/*
 * Page 3 register offsets.
 */
#define	NEPCI_RTL3_EECR		0x01	/* EEPROM Command Register */
#define	RTL3_EECR_EEM1		0x80	/* EEPROM Operating Mode */
#define	RTL3_EECR_EEM0		0x40	
					/* 0 0 Normal operation */
					/* 0 1 Auto-load */
					/* 1 0 9346 programming */
					/* 1 1 Config register write enab */
#define	RTL3_EECR_EECS		0x08	/* EEPROM Chip Select */
#define	RTL3_EECR_EESK		0x04	/* EEPROM Clock */
#define	RTL3_EECR_EEDI		0x02	/* EEPROM Data In */
#define	RTL3_EECR_EEDO		0x01	/* EEPROM Data Out */

#define	NEPCI_RTL3_CONFIG0	0x03	/* Configuration 0 (ro) */
#define	RTL3_CONFIG0_BNC	0x04	/* BNC is active */

#define	NEPCI_RTL3_CONFIG2	0x05	/* Configuration 2 */
#define	RTL3_CONFIG2_PL1	0x80	/* Network media type */
#define	RTL3_CONFIG2_PL0	0x40
					/* 0 0 TP/CX auto-detect */
					/* 0 1 10baseT */
					/* 1 0 10base5 */
					/* 1 1 10base2 */
#define	RTL3_CONFIG2_FCE	0x20	/* Flow Control Enable */
#define	RTL3_CONFIG2_PF		0x10	/* Pause Flag */
#define	RTL3_CONFIG2_BS1	0x02	/* Boot Rom Size */
#define	RTL3_CONFIG2_BS0	0x01
					/* 0 0 No Boot Rom */
					/* 0 1 8k */
					/* 1 0 16k */
					/* 1 1 32k */

#define	NEPCI_RTL3_CONFIG3	0x06	/* Configuration 3 */
#define	RTL3_CONFIG3_FUDUP	0x40	/* Full Duplex */
#define	RTL3_CONFIG3_LEDS1	0x20	/* LED1/2 pin configuration */
					/* 0 LED1 == LED_RX, LED2 == LED_TX */
					/* 1 LED1 == LED_CRS, LED2 == MCSB */
#define	RTL3_CONFIG3_LEDS0	0x10	/* LED0 pin configration */
					/* 0 LED0 pin == LED_COL */
					/* 1 LED0 pin == LED_LINK */
#define	RTL3_CONFIG3_SLEEP	0x04	/* Sleep mode */
#define	RTL3_CONFIG3_PWRDN	0x02	/* Power Down */

#define	NEPCI_RTL3_HLTCLK	0x09	/* Halt Clock */
#define	RTL3_HLTCLK_RUNNING	'R'	/* clock runs in power down */
#define	RTL3_HLTCLK_HALTED	'H'	/* clock halted in power down */

#define	NEPCI_RTL3_ID0		0x0e	/* ID register 0 */

#define	NEPCI_RTL3_ID1		0x0f	/* ID register 1 */
