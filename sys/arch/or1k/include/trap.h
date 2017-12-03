/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _OR1K_TRAP_H_
#define _OR1K_TRAP_H_

// OpenRISC 1000 Trap Vectors.
// These are named like the PowerPC ones since it seems that's
// where they stole them from.

#define	EXC_RESET	0x100
#define EXC_BUS		0x200	// EXC_MCHK on PowerPC
#define EXC_DFAULT	0x300	// EXC_DSI on PowerPC
#define EXC_IFAULT	0x400	// EXC_ISI on PowerPC
#define EXC_TICK	0x500	// EXC_DEC on PowerPC
#define EXC_ALI		0x600
#define EXC_PGM		0x700
#define EXC_EXI		0x800
#define EXC_DMISS	0x900
#define EXC_IMISS	0xa00
#define EXC_RANGE	0xb00	// Not on PowerPC
#define EXC_SC		0xc00
#define EXC_FP		0xd00	// EXC_FPA on PowerPC
#define EXC_TRAP	0xe00	// EXC_TRC on PowerPC

#endif /* _OR1K_TRAP_H_ */
