/*	$NetBSD: vme_pcc.c,v 1.4 1997/03/19 16:24:42 gwr Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * VME support specific to the Type 1 VMEchip found on the
 * MVME-147.
 *
 * For a manual on the MVME-147, call: 408.991.8634.  (Yes, this
 * is the Sunnyvale sales office.)
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/psl.h>
#include <machine/cpu.h>

#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/vme_pccreg.h>
#include <mvme68k/dev/vmevar.h>

int 	vmechip_pcc_match  __P((struct device *, struct cfdata *, void *));
void	vmechip_pcc_attach __P((struct device *, struct device *, void *));

struct cfattach vmechip_pcc_ca = {
	sizeof(struct vmechip_softc), vmechip_pcc_match, vmechip_pcc_attach
};

int	vmechip_pcc_translate_addr __P((u_long, size_t, int, int, u_long *));
void	vmechip_pcc_intrline_enable __P((int));
void	vmechip_pcc_intrline_disable __P((int));

struct vme_chip vme_pcc_switch = {
	vmechip_pcc_translate_addr,
	vmechip_pcc_intrline_enable,
	vmechip_pcc_intrline_disable
};

struct vme_pcc *sys_vme_pcc;

extern	int physmem;

int
vmechip_pcc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcc_attach_args *pa = aux;

	/* Only one VME chip, please. */
	if (sys_vme_pcc)
		return (0);

	if (strcmp(pa->pa_name, vmechip_cd.cd_name))
		return (0);

	return (1);
}

void
vmechip_pcc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct vmechip_softc *sc = (struct vmechip_softc *)self;
	struct pcc_attach_args *pa = aux;
	struct vme_pcc *vme;

	/* Glue into generic VME code. */
	sc->sc_reg = PCC_VADDR(pa->pa_offset);
	sc->sc_chip = &vme_pcc_switch;

	/* Initialize the chip. */
	vme = (struct vme_pcc *)sc->sc_reg;
	vme->vme_scon &= ~VME1_SCON_SYSFAIL;	/* XXX doesn't work */

	sys_vme_pcc = vme;

	printf(": Type 1 VMEchip, scon jumper %s\n",
	    (vme->vme_scon & VME1_SCON_SWITCH) ? "enabled" : "disabled");

	/* Attach children. */
	vme_config(sc);
}

int
vmechip_pcc_translate_addr(start, len, bustype, atype, addrp)
	u_long start;
	size_t len;
	int bustype, atype;
	u_long *addrp;		/* result */
{
	u_long end = (start + len) - 1;

	switch (bustype) {
	case VME_D16:
		switch (atype) {
		case VME_A16:
			if (end < VME1_A16D16_LEN) {
				*addrp = VME1_A16D16_START + start;
				return (0);
			}
			break;

		case VME_A24:
			if (((end & 0x00ffffff) == end) &&
			    (end < VME1_A32D16_LEN)) {
				*addrp = VME1_A32D16_START + start;
				return (0);
			}
			break;

		case VME_A32:
			if (end < VME1_A32D16_LEN) {
				*addrp = VME1_A32D16_START + start;
				return (0);
			}
			break;

		default:
			printf("vmechip: impossible atype `%d'\n", atype);
			panic("vmechip_pcc_translate_addr");
		}

		printf("vmechip: can't map %s atype %d addr 0x%lx len 0x%x\n", 
		    vmes_cd.cd_name, atype, start, len);
		break;

	case VME_D32:
		switch (atype) {
		case VME_A16:
			/* Can't map A16D32 */
			break;

		case VME_A24:
			if (((u_long)physmem < 0x1000000) &&	/* 16MB */
			    (start >= (u_long)physmem) &&
			    (end < VME1_A32D32_LEN)) {
				*addrp = start;
				return (0);
			}
			break;

		case VME_A32:
			if ((start >= (u_long)physmem) &&
			    (end < VME1_A32D32_LEN)) {
				*addrp = start;
				return (0);
			}
			break;

		default:
			printf("vmechip: impossible atype `%d'\n", atype);
			panic("vmechip_pcc_translate_addr");
		}

		printf("vmechip: can't map %s atype %d addr 0x%lx len 0x%x\n", 
		    vmel_cd.cd_name, atype, start, len);
		break;

	default:
		panic("vmechip_pcc_translate_addr: bad bustype");
	}

	return (1);
}

void
vmechip_pcc_intrline_enable(ipl)
	int ipl;
{

	sys_vme_pcc->vme_irqen |= VME1_IRQ_VME(ipl);
}

void
vmechip_pcc_intrline_disable(ipl)
	int ipl;
{

	sys_vme_pcc->vme_irqen &= ~VME1_IRQ_VME(ipl);
}
