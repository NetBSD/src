/*	$NetBSD: sa11x0_gpioreg.h,v 1.5.2.3 2001/04/21 17:53:36 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA.
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
 * SA-11x0 GPIO Register 
 */

#define SAGPIO_NPORTS	8

/* GPIO pin-level register */
#define SAGPIO_PLR	0x00

/* GPIO pin direction register */
#define SAGPIO_PDR	0x04

/* GPIO pin output set register */
#define SAGPIO_PSR	0x08

/* GPIO pin output clear register */
#define SAGPIO_PCR	0x0C

/* GPIO rising-edge detect register */
#define SAGPIO_RER	0x10

/* GPIO falling-edge detect register */
#define SAGPIO_FER	0x14

/* GPIO edge-detect status register */
#define SAGPIO_EDR	0x18

/* GPIO alternate function register */
#define SAGPIO_AFR	0x1C

/* XXX */
#define GPIO(x)		(0x00000001 << (x))

/*
 * iPAQ H3600 specific parameter
 */
/*
port	I/O(Active)	name 	desc
0	I(L)	PWR_ON#		button detect: power-on
1	I(L)	IP_IRQ#		cpu-interrupt
2...9	O	LDD{8..15}	LCD DATA(8-15)
10	I(L)	CARD_IND1#	PCMCIA Socket1 inserted detection
11	I(L)	CARD_IRQ1#	PCMCIA slot1 IRQ
12	O	CLK_SET0	clock select 0 for audio codec
13	O	CLK_SET1	clock select 1 for audio codec
14	I/O	L3_SDA		UDA1341 L3DATA
15	O	L3_MODE		UDA1341 L3MODE
16	O	L3_SCLK		UDA1341 L3SCLK
17	I(L)	CARD_IND0#	PCMCIA Socket0 inserted detection
18	I(L)	KEY_ACT#	button detect: center button
19	I	SYS_CLK		Stereo audio codev external clock
20	I(H)	BAT_FAULT	Battery fault
21	I(L)	CARD_IRQ0#	PCMCIA slot0 IRQ	
22	I(L)	LOCK#		expansion pack lock/unlock signal
23	I(H)	COM_DCD		RS-232 DCD
24	I(H)	OPT_IRQ		expansion pach shared IRQ
25	I(H)	COM_CTS		RS-232 CTS
26	O(H)	COM_RTS		RS-232 RTS
27	O(L)	OPT_DETECT#	Indicates presence of expansion pack inserted

Extended GPIO
0	O(H)	VPEN		Enables programming and erasing of Flash
1	O(H)	CARD_RESET	CF/PCMCIA card reset signal
2	O(H)	OPT_RESET	Expansion pack reset signal 
3	O(L)	CODEC_RESET#	onboard codec reset signal
4	O(H)	OPT_NVRAM_ON	Enables power supply to the NVRAM of the
				Expansion pack.(=OPT_ON)
5	O(H)	OPT_ON		Enables full power supply to the Expansion pack.
6	O(H)	LCD_ON		Enables LCD 3.3V power supply
7	O(H)	RS232_ON	Enables RS232
8	O(H)	LCD_PCI		Enables power to LCD control IC
9	O(H)	IR_ON		Enables power to IR module
10	O(H)	AUD_ON		Enables power to audio output amp.
11	O(H)	AUD_PWR_ON	Enables power to all audio modules.
12	O(H)	QMUTE		Mutes yhe onboard audio codec
13	O	IR_FSEL		FIR mode selection:H=FIR,L=SIR
14	O(H)	LCD_5V_ON	Enables 5V to the LCD module
15	O(H)	LVDD_ON		Enables 9V and -6.5V to the LCD module
 */

#define GPIO_H3600_POWER_BUTTON	GPIO (0)
#define GPIO_H3600_PCMCIA_CD0	GPIO (17)
#define GPIO_H3600_PCMCIA_CD1	GPIO (10)
#define GPIO_H3600_PCMCIA_IRQ0	GPIO (21)
#define GPIO_H3600_PCMCIA_IRQ1	GPIO (11)
#define GPIO_H3600_OPT_LOCK	GPIO (22)
#define GPIO_H3600_OPT_IRQ	GPIO (24)
#define GPIO_H3600_OPT_DETECT	GPIO (27)

#define IRQ_H3600_POWER_BUTTON	IRQ_GPIO0
#define IRQ_H3600_PCMCIA_CD0	IRQ_GPIO17
#define IRQ_H3600_PCMCIA_CD1	IRQ_GPIO10
#define IRQ_H3600_PCMCIA_IRQ0	IRQ_GPIO21
#define IRQ_H3600_PCMCIA_IRQ1	IRQ_GPIO11
#define IRQ_H3600_OPT_IRQ	IRQ_GPIO24
#define IRQ_H3600_OPT_DETECT	IRQ_GPIO27

/*
 * JORNADA720 specific parameter
 */

#define JORNADA720_KBD_IRQ	GPIO (0)
#define JORNADA720_MOUSE_IRQ	GPIO (9)
