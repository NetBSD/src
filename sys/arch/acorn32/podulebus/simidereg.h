/*	$NetBSD: simidereg.h,v 1.1 2001/10/05 22:27:59 reinoud Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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
 * Thanks to Gareth Simpson, Simtec Electronics for providing
 * the hardware information.
 */

/*
 * Registers and address offsets for the Simtec IDE card.
 */

/* IDE drive registers */

#define PRIMARY_DRIVE_REGISTERS_POFFSET		0x0000
#define PRIMARY_AUX_REGISTER_POFFSET		0x0700

#define SECONDARY_DRIVE_REGISTERS_POFFSET	0x1000
#define SECONDARY_AUX_REGISTER_POFFSET		0x1700

#define DRIVE_REGISTERS_SPACE			0x800
#define DRIVE_REGISTER_BYTE_SPACING		128
#define DRIVE_REGISTER_SPACING_SHIFT		7

/* Other registers */

#define CONTROL_REGISTERS_POFFSET		0x2000
#define CONTROL_REGISTER_SPACE			8
#define CONTROL_REGISTER_OFFSET			0
#define  CONTROL_RESET				0x80
#define	 CONTROL_IORDY				0x40
#define  CONTROL_8_BIT				0x20
#define	 CONTROL_IDE_ENABLE			0x10
#define  CONTROL_SLOW_MODE_OFF			0x08
#define	 CONTROL_ROM_WRITE			0x04
#define  CONTROL_SECONDARY_IRQ			0x02
#define  CONTROL_PRIMARY_IRQ			0x01

#define STATUS_REGISTER_OFFSET			1
#define  STATUS_RESET				0x80
#define  STATUS_IORDY				0x40
#define  STATUS_ADDR_TEST			0x20
#define  STATUS_CS_TEST				0x10
#define  STATUS_RW_TEST				0x08
#define  STATUS_IRQ				0x01

#define  STATUS_FAULT	(STATUS_ADDR_TEST | STATUS_CS_TEST \
			| STATUS_RW_TEST | STATUS_RESET)
