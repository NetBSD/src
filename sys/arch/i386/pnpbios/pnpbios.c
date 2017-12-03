/* $NetBSD: pnpbios.c,v 1.71.12.3 2017/12/03 11:36:18 jdolecek Exp $ */

/*
 * Copyright (c) 2000 Jason R. Thorpe.  All rights reserved.
 * Copyright (c) 2000 Christian E. Hopps.  All rights reserved.
 * Copyright (c) 1999
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * PnP BIOS documentation is available at the following locations.
 *
 * http://www.microsoft.com/hwdev/download/respec/pnpbios.zip
 * http://www.microsoft.com/hwdev/download/respec/biosclar.zip
 * http://www.microsoft.com/hwdev/download/resources/specs/devids.txt
 *
 * PNPBIOSEVENTS is unfinished.  After coding what I did I discovered
 * I had no platforms to test on so someone else will need to finish
 * it.  I didn't want to toss the code though
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pnpbios.c,v 1.71.12.3 2017/12/03 11:36:18 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/kthread.h>

#include <uvm/uvm_extern.h>

#include <machine/isa_machdep.h>
#include <machine/segments.h>

#include <dev/isa/isareg.h>
#include <dev/isapnp/isapnpreg.h>

#include <arch/i386/pnpbios/pnpbiosvar.h>
#include <arch/i386/pnpbios/pnpbiosreg.h>

#include "opt_pnpbios.h"
#include "locators.h"

#ifdef PNPBIOSVERBOSE
int	pnpbiosverbose = 1;
#else
int	pnpbiosverbose = 0;
#endif

#ifdef PNPBIOSDEBUG
#ifdef PNPBIOSDEBUG_VALUE
int	pnpbiosdebug = PNPBIOSDEBUG_VALUE;
#else
int	pnpbiosdebug = 1;
#endif
#define	DPRINTF(x) if (pnpbiosdebug) aprint_normal x
#else
#define	DPRINTF(x)
#endif

#ifdef PNPBIOSEVENTSDEBUG
#define	EDPRINTF(x) aprint_normal x
#else
#define	EDPRINTF(x)
#endif

struct pnpbios_softc {
	device_t	sc_dev;
	isa_chipset_tag_t	sc_ic;
	lwp_t		*sc_evthread;
	int		sc_version;
	int		sc_control;
#ifdef PNPBIOSEVENTS
	uint8_t	*	sc_evaddr;
	int		sc_threadrun;
	int		sc_docked;
#endif
};

#define	PNPGET4(p)	((p)[0] + ((p)[1] << 8) + \
			((p)[2] << 16) + ((p)[3] << 24))

/* bios calls */
#if 0
/* XXX these are not called */
static int	pnpbios_getapmtable(uint8_t *, size_t *);
static int	pnpbios_setnode(int, int,
			    const uint8_t *, size_t);
#endif

static int	pnpbios_getnode(int, int *,
			    uint8_t *, size_t);
static int	pnpbios_getnumnodes(int *, size_t *);

#ifdef PNPBIOSEVENTS
static int	pnpbios_getdockinfo(struct pnpdockinfo *);

static int	pnpbios_getevent(uint16_t *);
static void	pnpbios_event_thread(void *);
static int	pnpbios_sendmessage(int);
#endif

/* configuration stuff */
static void *	pnpbios_mapit(paddr_t, u_long, vm_prot_t);
static void *	pnpbios_find(void);
static int	pnpbios_match(device_t, cfdata_t, void *);
static void	pnpbios_attach(device_t, device_t, void *);
static void	pnpbios_printres(struct pnpresources *);
static int	pnpbios_print(void *aux, const char *);
static void	pnpbios_id_to_string(uint32_t, char *);
static int	pnpbios_attachnode(struct pnpbios_softc *,
			    int, const uint8_t *,
			    size_t, int);

static int	pnp_scan(const uint8_t **, size_t,
			struct pnpresources *, int);
extern int	pnpbioscall(int);

static void	pnpbios_enumerate(struct pnpbios_softc *);
#ifdef PNPBIOSEVENTS
static int	pnpbios_update_dock_status(struct pnpbios_softc *);
#endif

/* scanning functions */
static int pnp_compatid(struct pnpresources *, const void *, size_t);
static int pnp_newirq(struct pnpresources *, const void *, size_t);
static int pnp_newdma(struct pnpresources *, const void *, size_t);
static int pnp_newioport(struct pnpresources *, const void *, size_t);
static int pnp_newfixedioport(struct pnpresources *, const void *, size_t);
#ifdef PNPBIOSDEBUG
static int pnp_debugdump(struct pnpresources *, const void *, size_t);
#endif

/*
 * small ressource types (beginning with 1)
 */
static const struct{
	int (*handler)(struct pnpresources *, const void *, size_t);
	int minlen, maxlen;
} smallrescs[] = {
	{0, 2, 2}, /* PnP version number */
	{0, 5, 6}, /* logical device id */
	{pnp_compatid, 4, 4}, /* compatible device id */
	{pnp_newirq, 2, 3}, /* irq  descriptor */
	{pnp_newdma, 2, 2}, /* DMA  descriptor */
	{0, 0, 1}, /* start dep */
	{0, 0, 0}, /* end dep */
	{pnp_newioport, 7, 7}, /* io descriptor */
	{pnp_newfixedioport, 3, 3}, /* fixed io descriptor */
	{0, -1, -1}, /* reserved */
	{0, -1, -1},
	{0, -1, -1},
	{0, -1, -1},
	{0, 1, 7}, /* vendor defined */
	{0, 1, 1} /* end */
};


CFATTACH_DECL_NEW(pnpbios, sizeof(struct pnpbios_softc),
    pnpbios_match, pnpbios_attach, NULL, NULL);

/*
 * Private stack and return value buffer. Spec (1.0a, ch. 4.3) says that
 * 1024 bytes must be available to the BIOS function.
 */
#define PNPBIOS_BUFSIZE 4096

int pnpbios_enabled = 1;
size_t pnpbios_entry;
char *pnpbios_scratchbuf;

/*
 * There can be only one of these, and the i386 ISA code needs to
 * reference this.
 */
struct pnpbios_softc *pnpbios_softc;

#define PNPBIOS_SIGNATURE ('$' | ('P' << 8) | ('n' << 16) | ('P' << 24))

static void *
pnpbios_find(void)
{
	char *p, *c;
	uint8_t cksum;
	size_t structlen;

	for (p = (char *)ISA_HOLE_VADDR(0xf0000);
	     p <= (char *)ISA_HOLE_VADDR(0xffff0);
	     p += 16) {
		if (*(int *)p != PNPBIOS_SIGNATURE)
			continue;
		structlen = *(uint8_t *)(p + 5);
		if ((structlen < 0x21) ||
		    ((p + structlen - 1) > (char *)ISA_HOLE_VADDR(0xfffff)))
			continue;

		cksum = 0;
		for (c = p; c < p + structlen; c++)
			cksum += *(uint8_t *)c;
		if (cksum != 0)
			continue;

		if (*(char *)(p + 4) != 0x10) {
			printf("unknown version %x\n", *(char *)(p + 4));
			continue;
		}

		return (p);
	}

	return (0);
}

int
pnpbios_probe(void)
{

	return (pnpbios_find() != 0);
}

static int
pnpbios_match(device_t parent, cfdata_t match, void *aux)
{

	/* There can be only one! */
	if (pnpbios_softc != NULL)
		return (0);

	return (pnpbios_enabled);
}

static void *
pnpbios_mapit(paddr_t addr, u_long len, vm_prot_t prot)
{
	paddr_t startpa, pa, endpa;
	vaddr_t startva, va;

	pa = startpa = x86_trunc_page(addr);
	endpa = x86_round_page(addr + len);

	va = startva = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY);
	if (!startva)
		return (0);
	for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE)
		pmap_kenter_pa(va, pa, prot, 0);
	pmap_update(pmap_kernel());

	return ((void *)(startva + (vaddr_t)(addr - startpa)));
}

static void
pnpbios_attach(device_t parent, device_t self, void *aux)
{
	struct pnpbios_softc *sc = device_private(self);
	struct pnpbios_attach_args *paa = aux;
	char *p;
	unsigned int codepbase, datapbase, evaddrp;
	void *codeva, *datava;
	extern char pnpbiostramp[], epnpbiostramp[];
	int res, num, size;
#ifdef PNPBIOSEVENTS
	int evtype;
#endif

	aprint_naive("\n");

	pnpbios_softc = sc;

	/* We *don't* clamp xfer size here as both PCI and ISA devs
	   may attach beneath us */
	sc->sc_dev = self;
	sc->sc_ic = paa->paa_ic;

	p = pnpbios_find();
	if (!p)
		panic("pnpbios_attach: disappeared");

	sc->sc_version = *(uint8_t *)(p + 0x04);
	sc->sc_control = *(uint8_t *)(p + 0x06);
	evaddrp = *(uint32_t *)(p + 0x09);
	codepbase = *(uint32_t *)(p + 0x13);
	datapbase = *(uint32_t *)(p + 0x1d);
	pnpbios_entry = *(uint16_t *)(p + 0x11);

	if (pnpbiosverbose) {
		aprint_normal(": code %x, data %x, entry %x, control %x,"
			      " eventp %x\n%s",
		    codepbase, datapbase, pnpbios_entry, sc->sc_control,
		    (unsigned int)evaddrp, device_xname(self));
	}

#ifdef PNPBIOSEVENTS
	/* if we have an event mechnism queue a thread to deal with them */
	evtype = (sc->sc_control & PNP_IC_CONTORL_EVENT_MASK);
	if (evtype == PNP_IC_CONTROL_EVENT_POLL) {
		sc->sc_evaddr = pnpbios_mapit(evaddrp, PAGE_SIZE,
			VM_PROT_READ | VM_PROT_WRITE);
		if (!sc->sc_evaddr)
			aprint_error_dev(self, "couldn't map event flag 0x%08x\n",
			    evaddrp);
	}
#endif

	codeva = pnpbios_mapit(codepbase, 0x10000,
		VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	datava = pnpbios_mapit(datapbase, 0x10000,
		VM_PROT_READ | VM_PROT_WRITE);
	if (codeva == 0 || datava == 0) {
		aprint_error(": no vm for mapping\n");
		return;
	}
	pnpbios_scratchbuf = malloc(PNPBIOS_BUFSIZE, M_DEVBUF, M_NOWAIT);

	setsegment(&gdtstore[GPNPBIOSCODE_SEL].sd, codeva, 0xffff,
		   SDT_MEMERA, SEL_KPL, 0, 0);
	setsegment(&gdtstore[GPNPBIOSDATA_SEL].sd, datava, 0xffff,
		   SDT_MEMRWA, SEL_KPL, 0, 0);
	setsegment(&gdtstore[GPNPBIOSSCRATCH_SEL].sd,
		   pnpbios_scratchbuf, PNPBIOS_BUFSIZE - 1,
		   SDT_MEMRWA, SEL_KPL, 0, 0);
	setsegment(&gdtstore[GPNPBIOSTRAMP_SEL].sd,
		   pnpbiostramp, epnpbiostramp - pnpbiostramp - 1,
		   SDT_MEMERA, SEL_KPL, 1, 0);

	res = pnpbios_getnumnodes(&num, &size);
	if (res) {
		aprint_error(": pnpbios_getnumnodes: error %d\n", res);
		return;
	}

	aprint_normal(": nodes %d, max len %d\n", num, size);

#ifdef PNPBIOSEVENTS
	EDPRINTF(("%s: event flag vaddr 0x%08x\n", device_xname(self),
	    (int)sc->sc_evaddr));

	/* Set initial dock status. */
	sc->sc_docked = -1;
	(void) pnpbios_update_dock_status(sc);
#endif

	/* Enumerate the device nodes. */
	pnpbios_enumerate(sc);

#ifdef PNPBIOSEVENTS
	/* if we have an event mechnism queue a thread to deal with them */
	/* XXX need to update with irq if we do that */
	if (evtype != PNP_IC_CONTROL_EVENT_NONE) {
		if (evtype != PNP_IC_CONTROL_EVENT_POLL || sc->sc_evaddr) {
			sc->sc_threadrun = 1;
			config_pending_incr(sc->sc_dev);
			if (kthread_create(PRI_NONE, 0, NULL,
			    pnpbios_event_thread, sc, &sc->sc_evthread,
			    "%s", device_xname(self)))
			    	panic("pnpbios: create event thread");
		}
	}
#endif
}

static void
pnpbios_enumerate(struct pnpbios_softc *sc)
{
	int res, num, i, size, idx, dynidx;
	struct pnpdevnode *dn;
	uint8_t *buf;

	res = pnpbios_getnumnodes(&num, &size);
	if (res) {
		aprint_error_dev(sc->sc_dev, "pnpbios_getnumnodes: error %d\n",
		    res);
		return;
	}

	buf = malloc(size, M_DEVBUF, M_NOWAIT);
	if (buf == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to allocate node buffer\n");
		return;
	}

	/* 
	 * Loop through the list of indices getting data and match/attaching
	 * each as appropriate.
	 *
	 * Unfortunately, some BIOSes seem to have fatal bugs getting the
	 * dynamic (i.e. currently active) configuration, for instance some
	 * Sony VAIO laptops, including the PCG-Z505HE.  They don't have such a
	 * problem with that static (i.e. next boot time) configuration,
	 * however.  The workaround is to get the static configuration for all
	 * indices, and only get dynamic configuration for devices where the
	 * match is positive.
	 *
	 * This seems to work conveniently as the indices that cause
	 * crashes (and it seems to vary from machine to machine) do not
	 * seem to be for devices that NetBSD's pnpbios supports.
	 */

	idx = 0;
	for (i = 0; i < num && idx != 0xff; i++) {
		DPRINTF(("%s: getting info for index %d\n",
		    device_xname(sc->sc_dev), idx));

		dynidx = idx;

		res = pnpbios_getnode(PNP_CF_DEVCONF_STATIC, &idx, buf, size);
		if (res) {
			aprint_error_dev(sc->sc_dev, "index %d error %d "
			    "getting static configuration\n", idx, res);
			continue;
		}
		dn = (struct pnpdevnode *)buf;
		if (!pnpbios_attachnode(sc, dn->dn_handle, buf, dn->dn_size, 1)) {
			DPRINTF(("%s handle %d: no match from static config\n",
			    device_xname(sc->sc_dev), dn->dn_handle));
			continue;
		}

		res = pnpbios_getnode(PNP_CF_DEVCONF_DYNAMIC, &dynidx, buf, size);
		if (res) {
			aprint_error_dev(sc->sc_dev, "index %d error %d "
			    "getting dynamic configuration\n", dynidx, res);
			continue;
		}
		dn = (struct pnpdevnode *)buf;
		if (!pnpbios_attachnode(sc, dn->dn_handle, buf, dn->dn_size, 0)) {
			DPRINTF(("%s handle %d: no match from dynamic config\n",
			    device_xname(sc->sc_dev), dn->dn_handle));
			continue;
		}
	}
	if (i != num)
		aprint_error_dev(sc->sc_dev, "got only %d nodes\n", i);
	if (idx != 0xff)
		aprint_error_dev(sc->sc_dev, "last index %d\n", idx);

	free(buf, M_DEVBUF);
}

#ifdef PNPBIOSEVENTS
static int
pnpbios_update_dock_status(struct pnpbios_softc *sc)
{
	struct pnpdockinfo di;
	const char *when, *style;
	int res, odocked = sc->sc_docked;

	res = pnpbios_getdockinfo(&di);
	if (res == PNP_RC_SYSTEM_NOT_DOCKED) {
		sc->sc_docked = 0;
		if (odocked != sc->sc_docked)
			printf("%s: not docked\n", device_xname(sc->sc_dev));
	} else if (res) {
		EDPRINTF(("%s: dockinfo failed 0x%02x\n",
		    device_xname(sc->sc_dev), res));
	} else {
		sc->sc_docked = 1;
		if (odocked != sc->sc_docked) {
			char idstr[8];
			pnpbios_id_to_string(di.di_id, idstr);
			printf("%s: dock id %s", device_xname(sc->sc_dev), idstr);
			if (pnpbiosverbose) {
				if (di.di_serial != -1)
					printf(", serial number %d",
					    di.di_serial);
			}
			switch (di.di_cap & PNP_DI_DOCK_STYLE_MASK) {
			case PNP_DI_DOCK_STYLE_SUPRISE:
				style = "surprise";
				break;
			case PNP_DI_DOCK_STYLE_VCR:
				style = "controlled";
				break;
			default:
				style = "<style unknown>";
				break;
			}
			switch (di.di_cap & PNP_DI_DOCK_WHEN_MASK) {
			case PNP_DI_DOCK_WHEN_NO_POWER:
				when = "cold";
				break;
			case PNP_DI_DOCK_WHEN_SUSPENDED:
				when = "warm";
				break;
			case PNP_DI_DOCK_WHEN_RUNNING:
				when = "hot";
				break;
			case PNP_DI_DOCK_WHEN_RESERVED:
				when = "<reserved>";
				break;
			default:
				when = "<dock type unknown>";
					break;
			}
			printf(", %s %s docking\n", style, when);
		}
	}

	return (odocked);
}
#endif

static int
pnpbios_getnumnodes(int *nump, size_t *sizep)
{
	int res;
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 2; /* buffer offset for node size */
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for numnodes */
	*--help = PNP_FC_GET_NUM_NODES;

	res = pnpbioscall(((char *)help) - pnpbios_scratchbuf);
	if (res)
		return (res);

	*nump = *(short *)(pnpbios_scratchbuf + 0);
	*sizep = *(short *)(pnpbios_scratchbuf + 2);
	return (0);
}

static int
pnpbios_getnode(int flags, int *idxp, uint8_t *buf, size_t len)
{
	int res;
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = flags;
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 2; /* buffer offset for node data */
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for index in/out */
	*--help = PNP_FC_GET_DEVICE_NODE;

	*(short *)(pnpbios_scratchbuf + 0) = *idxp;

	res = pnpbioscall(((char *)help) - pnpbios_scratchbuf);
	if (res)
		return (res);

	*idxp = *(short *)(pnpbios_scratchbuf + 0);
	memcpy(buf, pnpbios_scratchbuf + 2, len);
	return (0);
}


#if 0
/* XXX - pnpbios_setnode() is never called. */

static int
pnpbios_setnode(int flags, int idx, const uint8_t *buf, size_t len)
{
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = flags;
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for node data */
	*--help = idx;
	*--help = PNP_FC_SET_DEVICE_NODE;

	memcpy(pnpbios_scratchbuf, buf, len);

	return (pnpbioscall(((void *)help) - pnpbios_scratchbuf));
}
#endif /* 0 */

#ifdef PNPBIOSEVENTS
static int
pnpbios_getevent(uint16_t *event)
{
	int res;
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for message data */
	*--help = PNP_FC_GET_EVENT;

	res = pnpbioscall(((void *)help) - pnpbios_scratchbuf);
	*event = pnpbios_scratchbuf[0] + (pnpbios_scratchbuf[1] << 8);
	return (res);
}

static int
pnpbios_sendmessage(int msg)
{
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = msg;
	*--help = PNP_FC_SEND_MESSAGE;

	return (pnpbioscall(((void *)help) - pnpbios_scratchbuf));
}

static int
pnpbios_getdockinfo(struct pnpdockinfo *di)
{
	int res;
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for dock info */
	*--help = PNP_FC_GET_DOCK_INFO;

	res = pnpbioscall(((void *)help) - pnpbios_scratchbuf);
	memcpy(di, pnpbios_scratchbuf, sizeof(*di));
	return (res);
}
#endif /* PNPBIOSEVENTS */

#if 0
/* XXX - pnpbios_getapmtable() is not called. */

/* XXX we don't support more than PNPBIOS_BUFSIZE - (stacklen + 2) */
static int
pnpbios_getapmtable(uint8_t *tab, size_t *len)
{
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);
	size_t origlen, stacklen;
	int res;

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 2; /* buffer offset for table */
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for length */
	*--help = PNP_FC_GET_APM_TABLE;

	origlen = *len;
	stacklen = (void *)help - pnpbios_scratchbuf;
	if (origlen > PNPBIOS_BUFSIZE - stacklen - 2)
		origlen = PNPBIOS_BUFSIZE - stacklen - 2;
	*(uint16_t *)(pnpbios_scratchbuf) = origlen;

	res = pnpbioscall(((void *)help) - pnpbios_scratchbuf);
	*len = *(uint16_t *)pnpbios_scratchbuf;
	if (res)
		return (res);
	if (origlen && *len > origlen) {
		printf("pnpbios: returned apm table exceed requested size\n");
		return (PNP_RC_BUFFER_TOO_SMALL);
	}
	memcpy(tab, pnpbios_scratchbuf + 2, *len);
	return (0);
}
#endif

static void
pnpbios_id_to_string(uint32_t pnpid, char *s)
{
	uint8_t *id;
	
	id = (uint8_t *)&pnpid;
	*s++ = 'A' + (id[0] >> 2) - 1;
	*s++ = 'A' + ((id[0] & 3) << 3) + (id[1] >> 5) - 1;
	*s++ = 'A' + (id[1] & 0x1f) - 1;
	*s++ = HEXDIGITS[id[2] >> 4];
	*s++ = HEXDIGITS[id[2] & 0x0f];
	*s++ = HEXDIGITS[id[3] >> 4];
	*s++ = HEXDIGITS[id[3] & 0x0f];
	*s = '\0';
}

static void
pnpbios_printres(struct pnpresources *r)
{
	struct pnp_mem *mem;
	struct pnp_io *io;
	struct pnp_irq *irq;
	struct pnp_dma *dma;
	int p = 0;

	mem = SIMPLEQ_FIRST(&r->mem);
	if (mem) {
		aprint_normal("mem");
		do {
			aprint_normal(" %x", mem->minbase);
			if (mem->len > 1)
				aprint_normal("-%x",
					      mem->minbase + mem->len - 1);
		} while ((mem = SIMPLEQ_NEXT(mem, next)));
		p++;
	}
	io = SIMPLEQ_FIRST(&r->io);
	if (io) {
		if (p++)
			aprint_normal(", ");
		aprint_normal("io");
		do {
			aprint_normal(" %x", io->minbase);
			if (io->len > 1)
				aprint_normal("-%x",
					      io->minbase + io->len - 1);
		} while ((io = SIMPLEQ_NEXT(io, next)));
	}
	irq = SIMPLEQ_FIRST(&r->irq);
	if (irq) {
		if (p++)
			aprint_normal(", ");
		aprint_normal("irq");
		do {
			aprint_normal(" %d", ffs(irq->mask) - 1);
		} while ((irq = SIMPLEQ_NEXT(irq, next)));
	}
	dma = SIMPLEQ_FIRST(&r->dma);
	if (dma) {
		if (p)
			aprint_normal(", ");
		aprint_normal("DMA");
		do {
			aprint_normal(" %d", ffs(dma->mask) - 1);
		} while ((dma = SIMPLEQ_NEXT(dma, next)));
	}
}

static int
pnpbios_print(void *aux, const char *pnp)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (pnp)
		return (QUIET);

	aprint_normal(" index %d (%s", aa->idx, aa->primid);
	if (aa->resc->longname)
		aprint_normal(", %s", aa->resc->longname);
	if (aa->idstr != aa->primid)
		aprint_normal(", attached as %s", aa->idstr);
	aprint_normal(")");

	return (0);
}

void
pnpbios_print_devres(device_t dev, struct pnpbiosdev_attach_args *aa)
{

	aprint_normal_dev(dev, "");
	pnpbios_printres(aa->resc);
	aprint_normal("\n");
}

static int
pnpbios_attachchild(struct pnpbios_softc *sc,
		    struct pnpbiosdev_attach_args *aa, int matchonly)
{
	int locs[PNPBIOSCF_NLOCS];

	locs[PNPBIOSCF_INDEX] = aa->idx;

	if (matchonly)
		return (config_search_loc(config_stdsubmatch, sc->sc_dev,
					 "pnpbios", locs, aa) != NULL);
	else 
		return (config_found_sm_loc(sc->sc_dev, "pnpbios",
			locs, aa, pnpbios_print, config_stdsubmatch)
				!= NULL);
}

static int
pnpbios_attachnode(struct pnpbios_softc *sc, int idx, const uint8_t *buf,
    size_t len, int matchonly)
{
	const struct pnpdevnode *dn;
	const uint8_t *p;
	char idstr[8];
	struct pnpresources r, s;
	struct pnpbiosdev_attach_args aa;
	struct pnp_compatid *compatid;
	int res, i;

	dn = (const struct pnpdevnode *)buf;
	pnpbios_id_to_string(dn->dn_product, idstr);
	p = (const u_char *)(dn + 1);

	DPRINTF(("%s (%s): type 0x%02x subtype "
	    "0x%02x dpi 0x%02x attr 0x%04x:\n",
	    idstr, matchonly ? "static" : "dynamic", dn->dn_type,
	    dn->dn_subtype, dn->dn_dpi, dn->dn_attr));
	DPRINTF(("%s: allocated config scan:\n", idstr));
	res = pnp_scan(&p, len - 12, &r, 0);
	if (res < 0) {
		aprint_error("error in config data\n");
		goto dump;
	}

	/*
	 * the following is consistency check only for now
	 */
	DPRINTF(("\tpossible config scan:\n"));
	res = pnp_scan(&p, len - (p - buf), &s, 0);
	if (res < 0) {
		aprint_error("error in possible configuration\n");
		goto dump;
	}

	DPRINTF(("\tcompat id scan:\n"));
	res = pnp_scan(&p, len - (p - buf), &s, 0);
	if (res < 0) {
		aprint_error("error in compatible ID\n");
		goto dump;
	}

	if (p != buf + len) {
		aprint_error_dev(sc->sc_dev, "length mismatch in node %d:"
			     " used %d of %d Bytes\n",
		       idx, p - buf, len);
		if (p > buf + len) {
			/* XXX shouldn't happen - pnp_scan should catch it */
			goto dump;
		}
		/* Crappy BIOS: Buffer is not fully used. Be generous. */
	}

	if (r.nummem + r.numio + r.numirq + r.numdma == 0) {
		if (pnpbiosverbose) {
			aprint_normal("%s", idstr);
			if (r.longname)
				aprint_normal(", %s", r.longname);
			compatid = s.compatids;
			while (compatid) {
				aprint_normal(", %s", compatid->idstr);
				compatid = compatid->next;
			}
			aprint_normal(" at %s index %d disabled\n",
			    device_xname(sc->sc_dev), idx);
		}
		return 0;
	}

	aa.pbt = 0; /* XXX placeholder */
	aa.idx = idx;
	aa.resc = &r;
	aa.ic = sc->sc_ic;
	aa.primid = idstr;

	/* first try the specific device ID */
	aa.idstr = idstr;
	if (pnpbios_attachchild(sc, &aa, matchonly))
		return -1;

	/* if no driver was found, try compatible IDs */
	compatid = s.compatids;
	while (compatid) {
		aa.idstr = compatid->idstr;
		if (pnpbios_attachchild(sc, &aa, matchonly))
			return -1;
		compatid = compatid->next;
	}

	if (pnpbiosverbose) {
		aprint_normal("%s", idstr);
		if (r.longname)
			aprint_normal(", %s", r.longname);
		compatid = s.compatids;
		while (compatid) {
			aprint_normal(", %s", compatid->idstr);
			compatid = compatid->next;
		}
		aprint_normal(" (");
		pnpbios_printres(&r);
		aprint_normal(") at %s index %d ignored\n",
			      device_xname(sc->sc_dev), idx);
	}

	return 0;

	/* XXX should free resource lists */

dump:
	i = 0;
#ifdef PNPBIOSDEBUG
	/* print some useful info */
	if (len >= sizeof(*dn)) {
		aprint_normal("%s idx %d size %d type 0x%x:0x%x:0x%x attr 0x%x\n",
		    idstr, dn->dn_handle, dn->dn_size, dn->dn_type,
		    dn->dn_subtype, dn->dn_dpi, dn->dn_attr);
		i += sizeof(*dn);
	}
#endif
	for (; i < len; i++)
		aprint_normal(" %02x", buf[i]);
	aprint_normal("\n");
	return 0;
}

static int
pnp_scan(const uint8_t **bufp, size_t maxlen,
    struct pnpresources *r, int in_depends)
{
	const void *start;
	const uint8_t *p;
	struct pnp_mem *mem;
	int tag, type, len;
	char *idstr;
	int i;

	p = *bufp;

	memset(r, 0, sizeof(*r));
	SIMPLEQ_INIT(&r->mem);
	SIMPLEQ_INIT(&r->io);
	SIMPLEQ_INIT(&r->irq);
	SIMPLEQ_INIT(&r->dma);

	for (;;) {
		if (p >= *bufp + maxlen) {
			aprint_normal("pnp_scanresources: end of buffer\n");
			return (-1);
		}
		start = p;
		tag = *p;
		if (tag & ISAPNP_LARGE_TAG) {
			len = *(const uint16_t *)(p + 1);
			p += sizeof(struct pnplargeres) + len;

			switch (tag) {
			case ISAPNP_TAG_MEM_RANGE_DESC: {
				const struct pnpmem16rangeres *res = start;
				if (len != sizeof(*res) - 3) {
					aprint_normal("pnp_scan: bad mem desc\n");
					return (-1);
				}

				mem = malloc(sizeof(struct pnp_mem),
					     M_DEVBUF, M_WAITOK);
				mem->flags = res->r_flags;
				mem->minbase = res->r_minbase << 8;
				mem->maxbase = res->r_maxbase << 8;
				mem->align = res->r_align;
				if (mem->align == 0)
					mem->align = 0x10000;
				mem->len = res->r_len << 8;
				DPRINTF(("\ttag memrange "));
				goto gotmem;
			}
			case ISAPNP_TAG_ANSI_IDENT_STRING: {
				const struct pnpansiidentres *res = start;
				if (in_depends)
					aprint_normal("ID in dep?\n");
				idstr = malloc(len + 1, M_DEVBUF, M_NOWAIT);
				for (i = 0; i < len; i++)
					idstr[i] = res->r_id[i];
				idstr[len] = '\0';

				DPRINTF(("\ttag ansiident %s\n", idstr));

				if (idstr[0] == '\0') {
					/* disabled device */
					free(idstr, M_DEVBUF);
					break;
				}
				r->longname = idstr;
				break;
			}
			case ISAPNP_TAG_MEM32_RANGE_DESC: {
				const struct pnpmem32rangeres *res = start;
				if (len != sizeof(*res) - 3) {
					aprint_normal("pnp_scan: bad mem32 desc\n");
					return (-1);
				}

				mem = malloc(sizeof(struct pnp_mem),
					     M_DEVBUF, M_WAITOK);
				mem->flags = res->r_flags;
				mem->minbase = res->r_minbase;
				mem->maxbase = res->r_maxbase;
				mem->align = res->r_align;
				mem->len = res->r_len;
				DPRINTF(("\ttag mem32range "));
				goto gotmem;
			}
			case ISAPNP_TAG_FIXED_MEM32_RANGE_DESC: {
				const struct pnpfixedmem32rangeres *res = start;
				if (len != sizeof(*res) - 3) {
					aprint_normal("pnp_scan: bad mem32 desc\n");
					return (-1);
				}

				mem = malloc(sizeof(struct pnp_mem),
					     M_DEVBUF, M_WAITOK);
				mem->flags = res->r_flags;
				mem->minbase = res->r_base;
				mem->maxbase = mem->minbase;
				mem->align = 0;
				mem->len = res->r_len;
				DPRINTF(("\ttag fixedmem32range "));
			gotmem:
				if (mem->len == 0) { /* disabled */
					DPRINTF(("zeroed\n"));
					free(mem, M_DEVBUF);
					break;
				}
				SIMPLEQ_INSERT_TAIL(&r->mem, mem, next);
				r->nummem++;

				DPRINTF(("flags %02x min %08x max %08x "
				    "align %08x len %08x\n", mem->flags,
				    mem->minbase, mem->maxbase, mem->align,
				    mem->len));

				break;
			}
			case ISAPNP_TAG_UNICODE_IDENT_STRING:
			case ISAPNP_TAG_VENDOR_DEFINED:
			default:
#ifdef PNPBIOSDEBUG
				pnp_debugdump(r, start, len);
#endif
				break;
			}
		} else {
			type = (tag >> 3) & 0x0f;
			len = tag & 0x07;
			p += 1 + len;

			if (type == 0 ||
			    len < smallrescs[type - 1].minlen ||
			    len > smallrescs[type - 1].maxlen) {
				aprint_normal("pnp_scan: bad small resource\n");
				return (-1);
			}
			if (type == ISAPNP_TAG_END) {
#ifdef PNPBIOSDEBUG
				const struct pnpendres *res = start;
#endif
				if (in_depends) {
					/*
					 * this seems to occur and is
					 * an optimization to not require
					 * the end dep in a depend
					 * that ends the section
					 */
					p -= 1 + len;
				}
				DPRINTF(("\ttag end cksum %02x\n",
				    res->r_cksum));
				break;
			}
			if (type == ISAPNP_TAG_DEP_START) {
#ifdef PNPBIOSDEBUG
				const struct pnpdepstartres *res = start;
#endif
				struct pnpresources *new, *last;
				int rv;

				DPRINTF(("\ttag startdep flags %02x\n",
				    len ? res->r_pri : ISAPNP_DEP_ACCEPTABLE));

				if (r->dependent_link) {
					aprint_normal("second dep?\n");
					return (-1);
				}
				/* XXX not sure about this */
				if (in_depends) {
					*bufp = p;
					return (1);
				}
				last = r;
				do {
					new = malloc(sizeof(*new),
						     M_DEVBUF, M_NOWAIT);

					rv = pnp_scan(&p, maxlen - (p - *bufp),
						       new, 1);
					if (rv < 0) {
						aprint_normal("error in"
						    " dependent function\n");
						free(new, M_DEVBUF);
						return (-1);
					}
					last->dependent_link = new;
					last = new;
				} while (rv > 0);
				continue;
			}
			if (type == ISAPNP_TAG_DEP_END) {
				DPRINTF(("\ttag enddep\n"));
				if (!in_depends) {
					aprint_normal("tag %d end dep?\n", tag);
					return (-1);
				}
				break;
			}
			if (!smallrescs[type - 1].handler) {
#ifdef PNPBIOSDEBUG
				pnp_debugdump(r, start, len);
#endif
			} else if (
			    (*smallrescs[type - 1].handler)(r, start, len))
				return (-1);
		}
	}
	*bufp = p;
	return (0);
}

static int
pnp_newirq(struct pnpresources *r, const void *vres, size_t len)
{
	const struct pnpirqres *res;
	struct pnp_irq *irq;

	res = vres;
	if (res->r_mask == 0) { /* disabled */
		DPRINTF(("\ttag irq zeroed\n"));
		return (0);
	}
	irq = malloc(sizeof(struct pnp_irq), M_DEVBUF, M_NOWAIT);
	irq->mask = res->r_mask;
	if (len > 2)
		irq->flags = res->r_info;
	else
		irq->flags = 0x01;
	SIMPLEQ_INSERT_TAIL(&r->irq, irq, next);
	r->numirq++;

	DPRINTF(("\ttag irq flags %02x mask %04x\n", irq->flags,irq->mask));

	return (0);
}

static int
pnp_newdma(struct pnpresources *r, const void *vres, size_t len)
{
	const struct pnpdmares *res;
	struct pnp_dma *dma;

	res = vres;
	if (res->r_mask == 0) { /* disabled */
		DPRINTF(("\ttag DMA zeroed\n"));
		return (0);
	}
	dma = malloc(sizeof(struct pnp_dma), M_DEVBUF, M_NOWAIT);
	dma->mask = res->r_mask;
	dma->flags = res->r_flags;
	SIMPLEQ_INSERT_TAIL(&r->dma, dma, next);
	r->numdma++;

	DPRINTF(("\ttag DMA flags %02x mask %02x\n", dma->flags,dma->mask));

	return (0);
}

static int
pnp_newioport(struct pnpresources *r, const void *vres, size_t len)
{
	const struct pnpportres *res;
	struct pnp_io *io;

	res = vres;
	if (res->r_len == 0) { /* disabled */
		DPRINTF(("\ttag io zeroed\n"));
		return (0);
	}
	io = malloc(sizeof(struct pnp_io), M_DEVBUF, M_NOWAIT);
	io->flags = res->r_flags;
	io->minbase = res->r_minbase;
	io->maxbase = res->r_maxbase;
	io->align = res->r_align;
	io->len = res->r_len;
	SIMPLEQ_INSERT_TAIL(&r->io, io, next);
	r->numio++;

	DPRINTF(("\ttag io flags %02x min %04x max %04x align "
	    "0x%02x len 0x%02x\n", io->flags, io->minbase, io->maxbase,
	    io->align, io->len));

	return (0);
}

static int
pnp_newfixedioport(struct pnpresources *r, const void *vres,
    size_t len)
{
	const struct pnpfixedportres *res;
	struct pnp_io *io;

	res = vres;
	if (res->r_len == 0) { /* disabled */
		DPRINTF(("\ttag fixedio zeroed\n"));
		return (0);
	}
	io = malloc(sizeof(struct pnp_io), M_DEVBUF, M_NOWAIT);
	io->flags = 1; /* 10 bit decoding */
	io->minbase = io->maxbase = res->r_base;
	io->align = 1;
	io->len = res->r_len;
	SIMPLEQ_INSERT_TAIL(&r->io, io, next);
	r->numio++;

	DPRINTF(("\ttag fixedio flags %02x base %04x align %02x len %02x\n",
	    io->flags, io->minbase, io->align, io->len));

	return (0);
}

static int
pnp_compatid(struct pnpresources *r, const void *vres, size_t len)
{
	const struct pnpcompatres *res;
	struct pnp_compatid *id;

	res = vres;
	id = malloc(sizeof(*id), M_DEVBUF, M_NOWAIT);
	pnpbios_id_to_string(res->r_id, id->idstr);
	id->next = r->compatids;
	r->compatids = id;

	DPRINTF(("\ttag compatid %s\n", id->idstr));

	return (0);
}

#ifdef PNPBIOSDEBUG
static int
pnp_debugdump(struct pnpresources *r, const void *vres, size_t len)
{
	const uint8_t *res = vres;
	int type, i;

	if (res[0] & ISAPNP_LARGE_TAG) {
		type = res[0] & 0x7f;
		aprint_normal("\tTAG %02x len %04x %s",
			      type, len, len ? "data" : "");
		i = 3;
	} else {
		type = (res[0] >> 3) & 0x0f;
		aprint_normal("\tTAG %02x len %02x %s",
			      type, len, len ? "data" : "");
		i = 1;
	}
	for (; i < len; i++)
		aprint_normal(" %02x", res[i]);
	aprint_normal("\n");

	return (0);
}
#endif

int
pnpbios_io_map(pnpbios_tag_t pbt, struct pnpresources *resc,
    int idx, bus_space_tag_t *tagp, bus_space_handle_t *hdlp)
{
	struct pnp_io *io;

	if (idx >= resc->numio)
		return (EINVAL);

	io = SIMPLEQ_FIRST(&resc->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	*tagp = x86_bus_space_io;
	return (bus_space_map(x86_bus_space_io, io->minbase, io->len,
			       0, hdlp));
}

void
pnpbios_io_unmap(pnpbios_tag_t pbt, struct pnpresources *resc,
    int idx, bus_space_tag_t tag, bus_space_handle_t hdl)
{
	struct pnp_io *io;

	if (idx >= resc->numio)
		return;

	io = SIMPLEQ_FIRST(&resc->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	bus_space_unmap(tag, hdl, io->len);
}

int
pnpbios_getiobase(pnpbios_tag_t pbt, struct pnpresources *resc,
    int idx, bus_space_tag_t *tagp, int *basep)
{
	struct pnp_io *io;

	if (idx >= resc->numio)
		return (EINVAL);

	io = SIMPLEQ_FIRST(&resc->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	if (tagp)
		*tagp = x86_bus_space_io;
	if (basep)
		*basep = io->minbase;
	return (0);
}

int
pnpbios_getiosize(pnpbios_tag_t pbt, struct pnpresources *resc,
    int idx, int *sizep)
{
        struct pnp_io *io;

        if (idx >= resc->numio)
            return (EINVAL);

        io = SIMPLEQ_FIRST(&resc->io);
        while (idx--)
                io = SIMPLEQ_NEXT(io, next);
        if (sizep)
                *sizep = io->len;
        return (0);
}

void *
pnpbios_intr_establish(pnpbios_tag_t pbt, struct pnpresources *resc,
    int idx, int level, int (*fcn)(void *), void *arg)
{
	struct pnp_irq *irq;
	int irqnum, type;

	if (idx >= resc->numirq)
		return (0);

	irq = SIMPLEQ_FIRST(&resc->irq);
	while (idx--)
		irq = SIMPLEQ_NEXT(irq, next);

	irqnum = ffs(irq->mask) - 1;
	type = (irq->flags & 0x0c) ? IST_LEVEL : IST_EDGE;

	return (isa_intr_establish(0, irqnum, type, level, fcn, arg));
}

int
pnpbios_getirqnum(pnpbios_tag_t pbt, struct pnpresources *resc,
    int idx, int *irqp, int *istp)
{
	struct pnp_irq *irq;

	if (idx >= resc->numirq)
		return (EINVAL);

	irq = SIMPLEQ_FIRST(&resc->irq);
	while (idx--)
		irq = SIMPLEQ_NEXT(irq, next);

	if (irqp != NULL)
		*irqp = ffs(irq->mask) - 1;
	if (istp != NULL)
		*istp = (irq->flags & 0x0c) ? IST_LEVEL : IST_EDGE;
	return (0);
}

int
pnpbios_getdmachan(pnpbios_tag_t pbt, struct pnpresources *resc,
    int idx, int *chanp)
{
	struct pnp_dma *dma;

	if (idx >= resc->numdma)
		return (EINVAL);

	dma = SIMPLEQ_FIRST(&resc->dma);
	while (idx--)
		dma = SIMPLEQ_NEXT(dma, next);

	*chanp = ffs(dma->mask) - 1;
	return (0);
}

#ifdef PNPBIOSEVENTS
static void
pnpbios_event_thread(void *arg)
{
	struct pnpbios_softc *sc;
	uint16_t event;
	u_int evflag;
	int rv, poll;

	sc = arg;
	if ((sc->sc_control & PNP_IC_CONTORL_EVENT_MASK)
	    != PNP_IC_CONTROL_EVENT_POLL)
		poll = 0;
	else {
		poll = hz;
		rv = pnpbios_sendmessage(PNP_CM_PNP_OS_ACTIVE);
		EDPRINTF(("pnpbios: os active returns 0x%02x\n", rv));
	}

	config_pending_decr(sc->sc_dev);

	goto start;
	while (sc->sc_threadrun) {
		/* maybe we have an event */
		if (!poll)
			(void)tsleep(pnpbios_event_thread, PWAIT,
			    "pnpbiosevent", 0);
		else if (((evflag = *sc->sc_evaddr) & 0x01) == 0) {
			if (evflag)
				EDPRINTF(("pnpbios: evflags 0x%02x\n", evflag));
			(void)tsleep(pnpbios_event_thread, PWAIT,
			    "pnpbiosevent", poll);
			continue;
		} else {
			EDPRINTF(("pnpbios: evflags 0x%02x\n", evflag));
		}
start:
		if ((rv = pnpbios_getevent(&event))) {
			EDPRINTF(("pnpbios: getevent rc: 0x%02x\n", rv));
#ifdef DIAGNOSTIC
			if (rv != PNP_RC_EVENTS_NOT_PENDING)
				printf("%s: getevent failed: %d\n",
				    device_xname(sc->sc_dev), rv);
#endif
			continue;
		}
		switch (event) {
		case PNP_EID_ABOUT_TO_CHANGE_CONFIG:
			EDPRINTF(("pnpbios: about to change event\n"));
			/*
			 * The system is about to be docked or undocked.
			 * Acknowledge the event, so that the procedure
			 * can continue.
			 * XXX When should we ever send an ABORT?
			 */
			pnpbios_sendmessage(PNP_RM_OK);
			break;
		case PNP_EID_DOCK_CHANGED:
		    {
			int odocked;

			EDPRINTF(("pnpbios: dock changed event\n"));

			odocked = pnpbios_update_dock_status(sc);
			if (odocked == sc->sc_docked)
				break;
			switch (sc->sc_docked) {
			case 0:
				/* We have been undocked. */
				/* XXX detach devices XXX */
				break;

			case 1:
				/* We have been docked. */
				/* XXX attach devices XXX */
				break;

			default:
				/* getdockinfo failed! */
				printf("%s: dock changed event, but unable "
				    "to get dock info; event ignored\n",
				    device_xname(sc->sc_dev));
			}
			break;
		    }
		case PNP_EID_SYSTEM_DEVICE_CHANGED:
			EDPRINTF(("pnpbios: system device changed event\n"));
			break;
		case PNP_EID_CONFIG_CHANGE_FAILED:
			EDPRINTF(("pnpbios: config changed event\n"));
			break;
		case PNP_EID_UNKNOWN_SYSTEM_EVENT:
#ifdef DIAGNOSTIC
			printf("%s: \"unknown system event\"\n",
			    device_xname(sc->sc_dev));
#endif
			break;
		default:
#ifdef DIAGNOSTIC
			if (event & PNP_EID_OEM_DEFINED_BIT)
				printf("%s: vendor defined event 0x%04x\n",
				    device_xname(sc->sc_dev), event);
			else
				printf("%s: unknown event 0x%04x\n",
				    device_xname(sc->sc_dev), event);
#endif
			break;
		}
	}

	pnpbios_sendmessage(PNP_CM_PNP_OS_INACTIVE);
	kthread_exit(0);
}
#endif	/* PNPBIOSEVENTS */
