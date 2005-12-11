/*	$NetBSD: rsidereg.h,v 1.2 2005/12/11 12:16:05 christos Exp $	*/

/*
 * Copyright (c) 2002 Chris Gilbert
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#define PRIMARY_DRIVE_REGISTERS_POFFSET		0x0302b800
#define PRIMARY_AUX_REGISTER_POFFSET		(PRIMARY_DRIVE_REGISTERS_POFFSET + 0x380)

#define SECONDARY_DRIVE_REGISTERS_POFFSET	0x0302bc00
#define SECONDARY_AUX_REGISTER_POFFSET		(SECONDARY_DRIVE_REGISTERS_POFFSET + 0x380)

#define DRIVE_REGISTERS_SPACE			(8 * 0x40)
#define DRIVE_REGISTER_BYTE_SPACING		(0x40)
#define DRIVE_REGISTER_SPACING_SHIFT		6

/* Other registers */
#define  CONTROL_SECONDARY_IRQ			IRQ_NEVENT1
#define  CONTROL_PRIMARY_IRQ			IRQ_NEVENT2
