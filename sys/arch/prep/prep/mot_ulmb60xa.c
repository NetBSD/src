/*	$NetBSD: mot_ulmb60xa.c,v 1.1.8.3 2002/06/23 17:39:54 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus J. Klein.
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

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/platform.h>
#include <machine/residual.h>

/*
 * CPU Type Register
 * These definitions should eventually be moved to a separate header file.
 */
#define	MOT_CPUTYPE_REG			(PREP_BUS_SPACE_IO + 0x0800)

/* Cache Size */
#define	MOT_CPUTYPE_CSIZE_SHIFT		0
#define	MOT_CPUTYPE_CSIZE_MASK		0x3
#define	MOT_CPUTYPE_CSIZE(ct) \
    (((ct) >> MOT_CPUTYPE_CSIZE_SHIFT) & MOT_CPUTYPE_CSIZE_MASK)

#define	MOT_CPUTYPE_CSIZE_512K		0
#define	MOT_CPUTYPE_CSIZE_256K		1
#define	MOT_CPUTYPE_CSIZE_RESERVED	2
#define	MOT_CPUTYPE_CSIZE_NONE		3

/* Board ID */
#define	MOT_CPUTYPE_BOARDID_SHIFT	4
#define	MOT_CPUTYPE_BOARDID_MASK	0xf
#define	MOT_CPUTYPE_BOARDID(ct) \
    (((ct) >> MOT_CPUTYPE_BOARDID_SHIFT) & MOT_CPUTYPE_BOARDID_MASK)

#define	MOT_CPUTYPE_BOARDID_ULTRA	4
#define	MOT_CPUTYPE_BOARDID_MTX		14


static int mot_ulmb60xa_match(struct platform *);
static void pci_intr_fixup_mot_ulmb60xa(int, int, int *);

struct platform platform_mot_ulmb60xa = {
	"BULL ESTRELLA (e0)         (e0)",	/* model */ /* XXX */
	mot_ulmb60xa_match,			/* match */
	prep_pci_get_chipset_tag_indirect,	/* pci_get_chipset_tag */
	pci_intr_fixup_mot_ulmb60xa,		/* pci_intr_fixup */
	init_intr_ivr,				/* init_intr */
	cpu_setup_unknown,			/* cpu_setup */
	reset_prep_generic,			/* reset */
	obiodevs_nodev,				/* obiodevs */
};

static int
mot_ulmb60xa_match(struct platform *p)
{
	uint8_t cputype;

	if (p == NULL || p->model == NULL)
		return 0;
	if (strcmp(res->VitalProductData.PrintableModel, p->model) != 0)
		return 0;
	/* Should possibly match against the FirmwareSupplier as well. */
	cputype = *(volatile uint8_t *)MOT_CPUTYPE_REG;
	__asm __volatile ("eieio; sync");
	if (MOT_CPUTYPE_BOARDID(cputype) != MOT_CPUTYPE_BOARDID_ULTRA)
		return 0;
	
	return 1;
}

static void
pci_intr_fixup_mot_ulmb60xa(int bus, int dev, int *line)
{

	if (bus != 0)
		return;

	switch (dev) {
	case 12:		/* NCR 53c810 */
		*line = 11;
		break;
	case 14:		/* DEC 21440 */
		*line = 9;
		break;
	case 16:		/* Slot #1 */
		*line = 9;
		break;
	case 17:		/* Slot #2 */
		*line = 9;
		break;
	case 18:		/* Slot #3 */
		*line = 11;
		break;
	}
}
