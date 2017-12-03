/*	$OpenBSD: isa_machdep.c,v 1.1 2010/05/08 21:59:56 miod Exp $	*/

/*
 * Copyright (c) 2009, 2010 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Legacy device support.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/cpuregs.h>
#include <evbmips/loongson/autoconf.h>
#include <machine/intr.h>

#include <dev/ic/i8259reg.h>

#include <dev/pci/pcivar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

#include <evbmips/loongson/loongson_isa.h>

uint	loongson_isaimr;

void
loongson_set_isa_imr(uint newimr)
{
	uint imr1, imr2;

	imr1 = 0xff & ~newimr;
	imr1 &= ~(1 << 2);	/* enable cascade */
	imr2 = 0xff & ~(newimr >> 8);

	/*
	 * For some reason, trying to write the same value to the PIC
	 * registers causes an immediate system freeze (at least on the
	 * 2F and CS5536 based Lemote Yeeloong), so we only do this if
	 * the value changes.
	 * Note that interrupts have been disabled by the caller.
	 */
	//if ((newimr ^ loongson_isaimr) & 0xff00)
		REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + 1) = imr2;
	//if ((newimr ^ loongson_isaimr) & 0x00ff)
		REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + 1) = imr1;
	__asm__ __volatile__ ("sync" ::: "memory");
	loongson_isaimr = newimr;
}

void
loongson_isa_specific_eoi(int bit)
{
	KASSERT((bit < 16) && (bit >= 0));
	loongson_isaimr &= ~(1 << bit);
	if (bit & 8) {
		(void)REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + 1);
		REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + 1) =
		    (0xff & ~(loongson_isaimr >> 8));
		__asm__ __volatile__ ("sync" ::: "memory");
		REGVAL8(BONITO_PCIIO_BASE + IO_ICU2 + PIC_OCW2) =
		    OCW2_SELECT | OCW2_EOI | OCW2_SL | OCW2_ILS(bit & 7);
		bit = 2;
		__asm__ __volatile__ ("sync" ::: "memory");
	}
	(void)REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + 1);
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + 1) =
	    (0xff & ~(loongson_isaimr));
	__asm__ __volatile__ ("sync" ::: "memory");
	REGVAL8(BONITO_PCIIO_BASE + IO_ICU1 + PIC_OCW2) =
	    OCW2_SELECT | OCW2_EOI | OCW2_SL | OCW2_ILS(bit);
	__asm__ __volatile__ ("sync" ::: "memory");
}
