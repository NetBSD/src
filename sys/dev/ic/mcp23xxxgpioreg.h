/*      $NetBSD: mcp23xxxgpioreg.h,v 1.1 2022/01/17 16:31:23 thorpej Exp $	*/

/*-
 * Copyright (c) 2014, 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Kardel, and by Jason R. Thorpe.
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

#ifndef _DEV_IC_MCP23xxxGPIOREG_H_
#define _DEV_IC_MCP23xxxGPIOREG_H_

/*
 * Microchip serial I/O expanders:
 *
 *	MCP23008	8-bit, I2C interface
 *	MCP23S08	8-bit, SPI interface
 *	MCP23017	16-bit, I2C interface
 *	MCP23S17	16-bit, SPI interface
 *	MCP23018	16-bit (open-drain outputs), I2C interface
 *	MCP23S18	16-bit (open-drain outputs), SPI interface
 *
 * Data sheet:
 *
 *	https://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
 */

/* resources */
#define	MCPGPIO_PINS_PER_BANK	8

#define	MCP23x08_GPIO_NBANKS	1
#define	MCP23x08_GPIO_NPINS	(MCPGPIO_PINS_PER_BANK * MCP23x08_GPIO_NBANKS)

#define	MCP23x17_GPIO_NBANKS	2
#define MCP23x17_GPIO_NPINS	(MCPGPIO_PINS_PER_BANK * MCP23x17_GPIO_NBANKS)

/*
 * The MCP23x17 has two addressing schemes, depending on the setting
 * of IOCON.BANK:
 *
 * IOCON.BANK=1		IOCON.BANK=0		Register
 * -----------------------------------------------------
 *	0x00			0x00		IODIRA
 *	0x10			0x01		IODIRB
 *	0x01			0x02		IPOLA
 *	0x11			0x03		IPOLB
 *	0x02			0x04		GPINTENA
 *	0x12			0x05		GPINTENB
 *	0x03			0x06		DEFVALA
 *	0x13			0x07		DEFVALB
 *	0x04			0x08		INTCONA
 *	0x14			0x09		INTCONB
 *	0x05			0x0a		IOCON
 *	0x15			0x0b		IOCON (yes, it's an alias)
 *	0x06			0x0c		GPPUA
 *	0x16			0x0d		GPPUB
 *	0x07			0x0e		INTFA
 *	0x17			0x0f		INTFB
 *	0x08			0x10		INTCAPA
 *	0x18			0x11		INTCAPB
 *	0x09			0x12		GPIOA
 *	0x19			0x13		GPIOB
 *	0x0a			0x14		OLATA
 *	0x1a			0x15		OLATB
 *
 * The MCP23x08, of course, only has a single bank of 8 GPIOs, and it
 * has an addressing schme that operates like IOCON.BANK=1
 */
#define	REG_IODIR		0x00
#define	REG_IPOL		0x01
#define	REG_GPINTEN		0x02
#define	REG_DEFVAL		0x03
#define	REG_INTCON		0x04
#define	REG_IOCON		0x05
#define	REG_GPPU		0x06
#define	REG_INTF		0x07
#define	REG_INTCAP		0x08
#define	REG_GPIO		0x09
#define	REG_OLAT		0x0a

/* IOCON.BANK=1 */
#define	REGADDR_BANK1(bank, reg)	(((bank) << 4) | (reg))

/* IOCON.BANK=0 */
#define	REGADDR_BANK0(bank, reg)	(((reg) << 1) | (bank))

/* bits */
#define IOCON_BANK	__BIT(7) /* select address layout (23x1x only) */
#define IOCON_MIRROR	__BIT(6) /* mirror INTA/INTB outputs (23x1x only) */
#define IOCON_SEQOP	__BIT(5) /* sequential address operation */
#define IOCON_DISLW	__BIT(4) /* slew rate SDA output */
#define IOCON_HAEN	__BIT(3) /* hardware address enable bit (SPI only) */
#define IOCON_ODR	__BIT(2) /* configure INT pin as open drain */
#define IOCON_INTPOL	__BIT(1) /* INT pin polarity (unless ODR is set) */

#endif /* _DEV_IC_MCP23xxxGPIOREG_H_ */
