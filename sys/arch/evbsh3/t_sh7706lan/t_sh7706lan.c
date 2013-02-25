/*	$NetBSD: t_sh7706lan.c,v 1.2.6.1 2013/02/25 00:28:40 tls Exp $	*/

/*-
 * Copyright (C) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: t_sh7706lan.c,v 1.2.6.1 2013/02/25 00:28:40 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sh3/devreg.h>
#include <sh3/bscreg.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>

void machine_init(void);

void
machine_init(void)
{
	uint16_t reg;

	/* IRQ0-4=IRQ */
	_reg_write_2(SH7709_PHCR, 0x2800);

	/* IRQ5=IRQ */
	reg = _reg_read_2(SH7709_SCPCR);
	reg &= 0x3ff;
	_reg_write_2(SH7709_SCPCR, reg);

	/* IRQ0-5=IRQ-mode, active-low */
	_reg_write_2(SH7709_ICR1, 0x0aaa);

	/* CS4: 8bit bus width */
	reg = _reg_read_2(SH3_BCR2);
	reg &= ~(BCR2_AREA_WIDTH_MASK << BCR2_AREA4_SHIFT);	
	reg |=  (BCR2_AREA_WIDTH_8 << BCR2_AREA4_SHIFT);
	_reg_write_2(SH3_BCR2, reg);
}
