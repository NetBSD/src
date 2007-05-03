/*	$NetBSD: openpic.h,v 1.5.32.1 2007/05/03 19:38:36 garbled Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/pio.h>

#include <machine/openpicreg.h>

extern volatile unsigned char *openpic_base;

static __inline u_int openpic_read __P((int));
static __inline void openpic_write __P((int, u_int));
static __inline int openpic_read_irq __P((int));
static __inline void openpic_eoi __P((int));

static __inline u_int
openpic_read(reg)
	int reg;
{
	volatile unsigned char *addr = openpic_base + reg;

	return in32rb(addr);
}

static __inline void
openpic_write(reg, val)
	int reg;
	u_int val;
{
	volatile unsigned char *addr = openpic_base + reg;

	out32rb(addr, val);
}

static __inline int
openpic_read_irq(cpu)
	int cpu;
{
	return openpic_read(OPENPIC_IACK(cpu)) & OPENPIC_VECTOR_MASK;
}

static __inline void
openpic_eoi(cpu)
	int cpu;
{
	openpic_write(OPENPIC_EOI(cpu), 0);
	openpic_read(OPENPIC_EOI(cpu));
}
