/*	$NetBSD: rapidereg.h,v 1.2 1997/03/15 18:09:42 is Exp $	*/

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
 *	This product includes software developed by Mark Brinicombe.
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
 * Thanks to Chris Honey at Raymond Datalink for providing information on
 * addressing the RapIDE podule.
 * RapIDE32 is Copyright (C) 1995,1996 Raymond Datalink. RapIDE32 is
 * manufactured under license by Yellowstone Educational Solutions.
 */

/*
 * Registers and address offsets for the Yellowstone RapIDE card.
 *
 * These are for issue 2 cards only.
 */

/*
 * This file needs to be extended to provide register information on
 * both versions of the card.
 */

/* IDE drive registers */

#define PRIMARY_DRIVE_REGISTERS_OFFSET		0x400080
#define PRIMARY_AUX_REGISTER_OFFSET		0x400280
#define PRIMARY_DATA_REGISTER_OFFSET		0x600080

#define SECONDARY_DRIVE_REGISTERS_OFFSET	0x400000
#define SECONDARY_AUX_REGISTER_OFFSET		0x400200
#define SECONDARY_DATA_REGISTER_OFFSET		0x600000

#define DRIVE_REGISTERS_SPACE			0x20
#define DRIVE_REGISTER_BYTE_SPACING		4

/* Other registers */

#define CONTROL_REGISTERS_OFFSET		0x200000
#define CONTROL_REGISTER_SPACE			16

#define IRQ_MASK_REGISTER_OFFSET		0
#define IRQ_STATUS_REGISTER_OFFSET		0
#define IRQ_REQUEST_REGISTER_OFFSET		1
#define IRQ_REQUEST_REGISTER_BYTE_OFFSET	(IRQ_REQUEST_REGISTER_OFFSET << 2)
#define IRQ_CLEAR_REGISTER_OFFSET		1
#define PRIMARY_IRQ_MASK			0x01
#define SECONDARY_IRQ_MASK			0x02
#define IRQ_MASK				(PRIMARY_IRQ_MASK | SECONDARY_IRQ_MASK)

#define VERSION_REGISTER_OFFSET			3
#define VERSION_REGISTER_MASK			0x03
#define VERSION_1_ID				0x00
#define VERSION_2_ID				0x01

#define PIO_MODE_CONTROL_REGISTER_OFFSET	3
#define PIO_MODE_0				0
#define PIO_MODE_1				1
#define PIO_MODE_2				2
#define PIO_MODE_3				3
#define PIO_MODE_4				4
