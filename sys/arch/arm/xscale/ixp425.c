/*	$NetBSD: ixp425.c,v 1.3 2003/05/24 01:59:32 ichiro Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425.c,v 1.3 2003/05/24 01:59:32 ichiro Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm.h>

#include <machine/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <evbarm/ixdp425/ixdp425reg.h>

static struct ixp425_softc *ixp425_softc;

void
ixp425_attach(struct ixp425_softc *sc)
{
	ixp425_softc = sc;

	printf("\n");
}

/*
 * IXP425 specific I/O registers mapping table
 */
static struct pmap_ent	map_tbl_ixp425[] = {
	{ "IXP425 Peripheral Registers",
	  IXP425_IO_VBASE, IXP425_IO_HWBASE,
	  IXP425_IO_SIZE,
	  VM_PROT_READ|VM_PROT_WRITE,
	  PTE_NOCACHE, },
	{ "IXP425 Expansion bus Registers",
	  IXP425_EXP_VBASE, IXP425_EXP_HWBASE,
	  IXP425_EXP_SIZE,
	  VM_PROT_READ|VM_PROT_WRITE,
	  PTE_NOCACHE, },
	{ "IXP425 PCI Configuration Registers",
	  IXP425_PCI_VBASE, IXP425_PCI_HWBASE,
	  IXP425_PCI_SIZE,
	  VM_PROT_READ|VM_PROT_WRITE,
	  PTE_NOCACHE, },

	{ NULL, 0, 0, 0, 0, 0 },
};

/*
 * mapping virtual memories
 */
void
ixp425_pmap_chunk_table(vaddr_t l1pt, struct pmap_ent* m)
{
	int loop;

	loop = 0;
	while (m[loop].msg) {
#ifdef DEBUG
		printf("mapping 0x%lx(0x%05lx) -> 0x%lx %s...\n",
			m[loop].pa, m[loop].sz, m[loop].va, m[loop].msg);
#endif
		pmap_map_chunk(l1pt, m[loop].va, m[loop].pa,
			       m[loop].sz, m[loop].prot, m[loop].cache);
		++loop;
	}
}

/*
 * mapping I/O registers
 */
void
ixp425_pmap_io_reg(vaddr_t l1pt)
{
	ixp425_pmap_chunk_table(l1pt, map_tbl_ixp425);
}
