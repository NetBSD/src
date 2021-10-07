/*	$NetBSD: x86_ipmi.c,v 1.3 2021/10/07 12:52:27 msaitoh Exp $ */

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *
 */

/*
 * Copyright (c) 2005 Jordan Hargrave
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: x86_ipmi.c,v 1.3 2021/10/07 12:52:27 msaitoh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/pmf.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/smbiosvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <x86/ipmivar.h>

#define SMBIOS_TYPE_IPMI	0x26

/*
 * Format of SMBIOS IPMI Flags
 *
 * bit0: interrupt trigger mode (1=level, 0=edge)
 * bit1: interrupt polarity (1=active high, 0=active low)
 * bit2: reserved
 * bit3: address LSB (1=odd,0=even)
 * bit4: interrupt (1=specified, 0=not specified)
 * bit5: reserved
 * bit6/7: register spacing (1,4,2,err)
 */
#define SMIPMI_FLAG_IRQLVL		(1L << 0)
#define SMIPMI_FLAG_IRQEN		(1L << 3)
#define SMIPMI_FLAG_ODDOFFSET		(1L << 4)
#define SMIPMI_FLAG_IFSPACING(x)	(((x)>>6)&0x3)
#define	 IPMI_IOSPACING_BYTE		 0
#define	 IPMI_IOSPACING_WORD		 2
#define	 IPMI_IOSPACING_DWORD		 1

static void
ipmi_smbios_probe(struct smbios_ipmi *pipmi, struct ipmi_attach_args *ia)
{
	const char *platform;

	aprint_debug("%s: %#.2x %#.2x %#.2x %#.2x %#08" PRIx64
	    " %#.2x %#.2x\n", __func__,
	    pipmi->smipmi_if_type,
	    pipmi->smipmi_if_rev,
	    pipmi->smipmi_i2c_address,
	    pipmi->smipmi_nvram_address,
	    pipmi->smipmi_base_address,
	    pipmi->smipmi_base_flags,
	    pipmi->smipmi_irq);

	ia->iaa_if_type = pipmi->smipmi_if_type;
	ia->iaa_if_rev = pipmi->smipmi_if_rev;
	ia->iaa_if_irq = (pipmi->smipmi_base_flags & SMIPMI_FLAG_IRQEN) ?
	    pipmi->smipmi_irq : -1;
	ia->iaa_if_irqlvl = (pipmi->smipmi_base_flags & SMIPMI_FLAG_IRQLVL) ?
	    IST_LEVEL : IST_EDGE;

	switch (SMIPMI_FLAG_IFSPACING(pipmi->smipmi_base_flags)) {
	case IPMI_IOSPACING_BYTE:
		ia->iaa_if_iospacing = 1;
		break;

	case IPMI_IOSPACING_DWORD:
		ia->iaa_if_iospacing = 4;
		break;

	case IPMI_IOSPACING_WORD:
		ia->iaa_if_iospacing = 2;
		break;

	default:
		ia->iaa_if_iospacing = 1;
		aprint_error("%s: unknown register spacing\n", __func__);
	}

	/* Calculate base address (PCI BAR format) */
	if (pipmi->smipmi_base_address & 0x1) {
		ia->iaa_if_iotype = 'i';
		ia->iaa_if_iobase = pipmi->smipmi_base_address & ~0x1;
	} else {
		ia->iaa_if_iotype = 'm';
		ia->iaa_if_iobase = pipmi->smipmi_base_address & ~0xF;
	}
	if (pipmi->smipmi_base_flags & SMIPMI_FLAG_ODDOFFSET)
		ia->iaa_if_iobase++;

	platform = pmf_get_platform("system-product");
	if (platform != NULL &&
	    strcmp(platform, "ProLiant MicroServer") == 0 &&
	    pipmi->smipmi_base_address != 0) {
		ia->iaa_if_iospacing = 1;
		ia->iaa_if_iobase = pipmi->smipmi_base_address & ~0x7;
		ia->iaa_if_iotype = 'i';
		return;
	}

	if (pipmi->smipmi_base_flags == 0x7f) {
		/* IBM 325 eServer workaround */
		ia->iaa_if_iospacing = 1;
		ia->iaa_if_iobase = pipmi->smipmi_base_address;
		ia->iaa_if_iotype = 'i';
		return;
	}
}

/* Scan memory for signature */
static void *
scan_sig(long start, long end, int skip, int len, const void *data)
{
	void *va;

	while (start < end) {
		va = ISA_HOLE_VADDR(start);
		if (memcmp(va, data, len) == 0)
			return va;

		start += skip;
	}

	return NULL;
}

int
ipmi_probe(struct ipmi_attach_args *ia)
{
	struct dmd_ipmi *pipmi;
	struct smbtable tbl;

	tbl.cookie = 0;

	if (smbios_find_table(SMBIOS_TYPE_IPMIDEV, &tbl))
		ipmi_smbios_probe(tbl.tblhdr, ia);
	else {
		pipmi = scan_sig(0xC0000L, 0xFFFFFL, 16, 4, "IPMI");
		/* XXX hack to find Dell PowerEdge 8450 */
		if (pipmi == NULL) {
			/* no IPMI found */
			return 0;
		}

		/* we have an IPMI signature, fill in attach arg structure */
		ia->iaa_if_type = pipmi->dmd_if_type;
		ia->iaa_if_rev = pipmi->dmd_if_rev;
	}

	return 1;
}

