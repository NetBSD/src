/* $NetBSD: pnpbios.c,v 1.20.4.3 2001/01/05 17:34:34 bouyer Exp $ */

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
 * http://www.microsoft.com/hwdev/download/respec/devids.txt
 *
 * PNPBIOSEVENTS is unfinished.  After coding what I did I discovered
 * I had no platforms to test on so someone else will need to finish
 * it.  I didn't want to toss the code though
 */

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

#include "opt_pnpbiosverbose.h"
#include "isadma.h"
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
#define	DPRINTF(x) if (pnpbiosdebug) printf x
#else
#define	DPRINTF(x)
#endif

#ifdef PNPBIOSEVENTSDEBUG
#define	EDPRINTF(x) printf x
#else
#define	EDPRINTF(x)
#endif

struct pnpbios_softc {
	struct device		sc_dev;
	isa_chipset_tag_t	sc_ic;
	struct proc		*sc_evthread;

	u_int8_t	*sc_evaddr;
	int		sc_version;
	int		sc_control;
	int		sc_threadrun;
	int		sc_docked;
};

#define	PNPGET4(p)	((p)[0] + ((p)[1] << 8) + \
			((p)[2] << 16) + ((p)[3] << 24))

/* bios calls */
#if 0
/* XXX these are not called */
static int	pnpbios_getapmtable	__P((u_int8_t *tab, size_t *len));
static int	pnpbios_setnode		__P((int flags, int idx,
    const u_int8_t *buf, size_t len));
#endif

static int	pnpbios_getdockinfo	__P((struct pnpdockinfo *di));
static int	pnpbios_getnode		__P((int flags, int *idxp,
    u_int8_t *buf, size_t len));
static int	pnpbios_getnumnodes	__P((int *nump, size_t *sizep));

#ifdef PNPBIOSEVENTS
static void	pnpbios_create_event_thread	__P((void *arg));
static int	pnpbios_getevent		__P((u_int16_t *event));
static void	pnpbios_event_thread		__P((void *arg));
static int	pnpbios_sendmessage		__P((int msg));
#endif

/* configuration stuff */
static caddr_t	pnpbios_mapit		__P((u_long addr, u_long len, 
    int prot));
static caddr_t	pnpbios_find		__P((void));
static int	pnpbios_match		__P((struct device *parent,
    struct cfdata *match, void *aux));
static void	pnpbios_attach		__P((struct device *parent,
    struct device *self, void *aux));
static void	pnpbios_printres	__P((struct pnpresources *r));
static int	pnpbios_print		__P((void *aux, const char *pnp));
static void	pnpbios_id_to_string	__P((u_int32_t pnpid, char *s));
static int	pnpbios_attachnode	__P((struct pnpbios_softc *sc,
    int idx, const u_int8_t *buf, size_t len, int matchonly));

static int	pnp_scan		__P((const u_int8_t **bufp,
    size_t maxlen, struct pnpresources *pnpresources, int in_depends));
static int	pnpbios_submatch	__P((struct device *parent,
    struct cfdata *match, void *aux));
extern int	pnpbioscall		__P((int));

static int	pnpbios_update_dock_status __P((struct pnpbios_softc *sc));

/* scanning functions */
static int pnp_compatid __P((struct pnpresources *, const void *, size_t));
static int pnp_newirq __P((struct pnpresources *, const void *, size_t));
static int pnp_newdma __P((struct pnpresources *, const void *, size_t));
static int pnp_newioport __P((struct pnpresources *, const void *, size_t));
static int pnp_newfixedioport __P((struct pnpresources *, const void *, size_t));
#ifdef PNPBIOSDEBUG
static int pnp_debugdump __P((struct pnpresources *, const void *, size_t));
#endif

/*
 * small ressource types (beginning with 1)
 */
static struct{
	int (*handler) __P((struct pnpresources *, const void *, size_t));
	int minlen, maxlen;
} smallrescs[] = {
	{0, 2, 2}, /* PnP version number */
	{0, 5, 6}, /* logical device id */
	{pnp_compatid, 4, 4}, /* compatible device id */
	{pnp_newirq, 2, 3}, /* irq  descriptor */
	{pnp_newdma, 2, 2}, /* dma  descriptor */
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


struct cfattach pnpbios_ca = {
	sizeof(struct pnpbios_softc), pnpbios_match, pnpbios_attach
};

/*
 * Private stack and return value buffer. Spec (1.0a, ch. 4.3) says that
 * 1024 bytes must be available to the BIOS function.
 */
#define PNPBIOS_BUFSIZE 4096

int pnpbios_enabled = 1;
size_t pnpbios_entry;
caddr_t pnpbios_scratchbuf;

/*
 * There can be only one of these, and the i386 ISA code needs to
 * reference this.
 */
struct pnpbios_softc *pnpbios_softc;

#define PNPBIOS_SIGNATURE ('$' | ('P' << 8) | ('n' << 16) | ('P' << 24))

static caddr_t
pnpbios_find()
{
	caddr_t p, c;
	u_int8_t cksum;
	size_t structlen;

	for (p = (caddr_t)ISA_HOLE_VADDR(0xf0000);
	     p <= (caddr_t)ISA_HOLE_VADDR(0xffff0);
	     p += 16) {
		if (*(int *)p != PNPBIOS_SIGNATURE)
			continue;
		structlen = *(u_int8_t *)(p + 5);
		if ((structlen < 0x21) ||
		    ((p + structlen - 1) > (caddr_t)ISA_HOLE_VADDR(0xfffff)))
			continue;

		cksum = 0;
		for (c = p; c < p + structlen; c++)
			cksum += *(u_int8_t *)c;
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
pnpbios_probe()
{

	return (pnpbios_find() != 0);
}

static int
pnpbios_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pnpbios_attach_args *paa = aux;

	/* These are not the droids you're looking for. */
	if (strcmp(paa->paa_busname, "pnpbios") != 0)
		return (0);

	/* There can be only one! */
	if (pnpbios_softc != NULL)
		return (0);

	return (pnpbios_enabled);
}

static caddr_t
pnpbios_mapit(addr, len, prot)
	u_long addr, len;
	int prot;
{
	u_long startpa, pa, endpa;
	vaddr_t startva, va;

	pa = startpa = i386_trunc_page(addr);
	endpa = i386_round_page(addr + len);

	va = startva = uvm_km_valloc(kernel_map, endpa - startpa);
	if (!startva)
		return (0);
	for (; pa < endpa; pa += NBPG, va += NBPG)
		pmap_kenter_pa(va, pa, prot);

	return ((caddr_t)(startva + (addr - startpa)));
}

static void
pnpbios_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pnpbios_softc *sc = (struct pnpbios_softc *)self;
	struct pnpbios_attach_args *paa = aux;
	struct pnpdevnode *dn;
	caddr_t p;
	unsigned int codepbase, datapbase, evaddrp;
	caddr_t codeva, datava;
	extern char pnpbiostramp[], epnpbiostramp[];
	int res, num, i, size, idx;
#ifdef PNPBIOSEVENTS
	int evtype;
#endif
	u_int8_t *buf;

	pnpbios_softc = sc;
	sc->sc_ic = paa->paa_ic;

#if NISADMA > 0
	isa_dmainit(sc->sc_ic, I386_BUS_SPACE_IO, &isa_bus_dma_tag, self);
#endif

	p = pnpbios_find();
	if (!p)
		panic("pnpbios_attach: disappeared");

	sc->sc_version = *(u_int8_t *)(p + 0x04);
	sc->sc_control = *(u_int8_t *)(p + 0x06);
	evaddrp = *(u_int32_t *)(p + 0x09);
	codepbase = *(u_int32_t *)(p + 0x13);
	datapbase = *(u_int32_t *)(p + 0x1d);
	pnpbios_entry = *(u_int16_t *)(p + 0x11);

	if (pnpbiosverbose) {
		printf(": code %x, data %x, entry %x, control %x eventp %x\n%s",
		    codepbase, datapbase, pnpbios_entry, sc->sc_control,
		    (int)evaddrp, self->dv_xname);
	}

#ifdef PNPBIOSEVENTS
	/* if we have an event mechnism queue a thread to deal with them */
	evtype = (sc->sc_control & PNP_IC_CONTORL_EVENT_MASK);
	if (evtype == PNP_IC_CONTROL_EVENT_POLL) {
		sc->sc_evaddr = pnpbios_mapit(evaddrp, NBPG,
			VM_PROT_READ | VM_PROT_WRITE);
		if (!sc->sc_evaddr)
			printf("%s: couldn't map event flag 0x%08x\n",
			    sc->sc_dev.dv_xname, evaddrp);
	}
#endif

	codeva = pnpbios_mapit(codepbase, 0x10000,
		VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	datava = pnpbios_mapit(datapbase, 0x10000,
		VM_PROT_READ | VM_PROT_WRITE);
	if (codeva == 0 || datava == 0) {
		printf("no vm for mapping\n");
		return;
	}
	pnpbios_scratchbuf = malloc(PNPBIOS_BUFSIZE, M_DEVBUF, M_NOWAIT);

	setsegment(&gdt[GPNPBIOSCODE_SEL].sd, codeva, 0xffff,
		   SDT_MEMERA, SEL_KPL, 0, 0);
	setsegment(&gdt[GPNPBIOSDATA_SEL].sd, datava, 0xffff,
		   SDT_MEMRWA, SEL_KPL, 0, 0);
	setsegment(&gdt[GPNPBIOSSCRATCH_SEL].sd,
		   pnpbios_scratchbuf, PNPBIOS_BUFSIZE - 1,
		   SDT_MEMRWA, SEL_KPL, 0, 0);
	setsegment(&gdt[GPNPBIOSTRAMP_SEL].sd,
		   pnpbiostramp, epnpbiostramp - pnpbiostramp - 1,
		   SDT_MEMERA, SEL_KPL, 1, 0);

	res = pnpbios_getnumnodes(&num, &size);
	if (res) {
		printf("pnpbios_getnumnodes: error %d\n", res);
		return;
	}

	printf(": nodes %d, max len %d\n", num, size);
	buf = malloc(size, M_DEVBUF, M_NOWAIT);

#ifdef PNPBIOSEVENTS
	EDPRINTF(("%s: event flag vaddr 0x%08x\n", sc->sc_dev.dv_xname,
	    (int)sc->sc_evaddr));
#endif

	/* Set initial dock status. */
	sc->sc_docked = -1;
	(void) pnpbios_update_dock_status(sc);

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
		int node = idx;
		int dynidx;

		DPRINTF(("%s: getting info for index %d\n",
		    sc->sc_dev.dv_xname, node));

		res = pnpbios_getnode(PNP_CF_DEVCONF_STATIC, &idx, buf, size);
		if (res) {
			printf("%s: index %d error %d "
			    "getting static configuration\n",
			    sc->sc_dev.dv_xname, node, res);
			continue;
		}
		dn = (struct pnpdevnode *)buf;
		if (dn->dn_handle != node)
			printf("%s: node index mismatch (static): "
			    "requested %d, got %d\n", sc->sc_dev.dv_xname,
			    node, dn->dn_handle);
		if (!pnpbios_attachnode(sc, node, buf, dn->dn_size, 1)) {
			DPRINTF(("%s index %d: no match from static config\n",
			    sc->sc_dev.dv_xname, node));
			continue;
		}
		dynidx = node;
		res = pnpbios_getnode(PNP_CF_DEVCONF_DYNAMIC, &dynidx, buf,
		    size);
		if (res) {
			printf("%s: index %d error %d "
			    "getting dynamic configuration\n",
			    sc->sc_dev.dv_xname, node, res);
			continue;
		}
		dn = (struct pnpdevnode *)buf;
		if (dn->dn_handle != node)
			printf("%s: node index mismatch (dynamic): "
			    "requested %d, got %d\n", sc->sc_dev.dv_xname,
			    node, dn->dn_handle);
		pnpbios_attachnode(sc, node, buf, dn->dn_size, 0);
	}
	if (i != num)
		printf("%s: got only %d nodes\n", sc->sc_dev.dv_xname, i);
	if (idx != 0xff)
		printf("%s: last index %d\n", sc->sc_dev.dv_xname, idx);

	free(buf, M_DEVBUF);

#ifdef PNPBIOSEVENTS
	/* if we have an event mechnism queue a thread to deal with them */
	/* XXX need to update with irq if we do that */
	if (evtype != PNP_IC_CONTROL_EVENT_NONE) {
		if (evtype != PNP_IC_CONTROL_EVENT_POLL || sc->sc_evaddr) {
			sc->sc_threadrun = 1;
			config_pending_incr();
			kthread_create(pnpbios_create_event_thread, sc);
		}
	}
#endif
}

static int
pnpbios_update_dock_status(sc)
	struct pnpbios_softc *sc;
{
	struct pnpdockinfo di;
	const char *when, *style;
	int res, odocked = sc->sc_docked;

	res = pnpbios_getdockinfo(&di);
	if (res == PNP_RC_SYSTEM_NOT_DOCKED) {
		sc->sc_docked = 0;
		if (odocked != sc->sc_docked)
			printf("%s: not docked\n", sc->sc_dev.dv_xname);
	} else if (res) {
		EDPRINTF(("%s: dockinfo failed 0x%02x\n",
		    sc->sc_dev.dv_xname, res));
	} else {
		sc->sc_docked = 1;
		if (odocked != sc->sc_docked) {
			char idstr[8];
			pnpbios_id_to_string(di.di_id, idstr);
			printf("%s: dock id %s", sc->sc_dev.dv_xname, idstr);
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
			}
			printf(", %s %s docking\n", style, when);
		}
	}

	return (odocked);
}

static int
pnpbios_getnumnodes(nump, sizep)
	int *nump;
	size_t *sizep;
{
	int res;
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 2; /* buffer offset for node size */
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for numnodes */
	*--help = PNP_FC_GET_NUM_NODES;

	res = pnpbioscall(((caddr_t)help) - pnpbios_scratchbuf);
	if (res)
		return (res);

	*nump = *(short *)(pnpbios_scratchbuf + 0);
	*sizep = *(short *)(pnpbios_scratchbuf + 2);
	return (0);
}

static int
pnpbios_getnode(flags, idxp, buf, len)
	int flags;
	int *idxp;
	u_int8_t *buf;
	size_t len;
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

	res = pnpbioscall(((caddr_t)help) - pnpbios_scratchbuf);
	if (res)
		return (res);

	*idxp = *(short *)(pnpbios_scratchbuf + 0);
	bcopy(pnpbios_scratchbuf + 2, buf, len);
	return (0);
}


#if 0
/* XXX - pnpbios_setnode() is never called. */

static int
pnpbios_setnode(flags, idx, buf, len)
	int flags, idx;
	const u_int8_t *buf;
	size_t len;
{
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = flags;
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for node data */
	*--help = idx;
	*--help = PNP_FC_SET_DEVICE_NODE;

	memcpy(pnpbios_scratchbuf, buf, len);

	return (pnpbioscall(((caddr_t)help) - pnpbios_scratchbuf));
}
#endif /* 0 */

#ifdef PNPBIOSEVENTS
static int
pnpbios_getevent(event)
	u_int16_t *event;
{
	int res;
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for message data */
	*--help = PNP_FC_GET_EVENT;

	res = pnpbioscall(((caddr_t)help) - pnpbios_scratchbuf);
	*event = pnpbios_scratchbuf[0] + (pnpbios_scratchbuf[1] << 8);
	return (res);
}

static int
pnpbios_sendmessage(msg)
	int msg;
{
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = msg;
	*--help = PNP_FC_SEND_MESSAGE;

	return (pnpbioscall(((caddr_t)help) - pnpbios_scratchbuf));
}
#endif /* PNPBIOSEVENTS */

static int
pnpbios_getdockinfo(di)
	struct pnpdockinfo *di;
{
	int res;
	short *help = (short *)(pnpbios_scratchbuf + PNPBIOS_BUFSIZE);

	*--help = GSEL(GPNPBIOSDATA_SEL, SEL_KPL);
	*--help = GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL);
	*--help = 0; /* buffer offset for dock info */
	*--help = PNP_FC_GET_DOCK_INFO;

	res = pnpbioscall(((caddr_t)help) - pnpbios_scratchbuf);
	memcpy(di, pnpbios_scratchbuf, sizeof(*di));
	return (res);
}

#if 0
/* XXX - pnpbios_getapmtable() is not called. */

/* XXX we don't support more than PNPBIOS_BUFSIZE - (stacklen + 2) */
static int
pnpbios_getapmtable(tab, len)
	u_int8_t *tab;
	size_t *len;
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
	stacklen = (caddr_t)help - pnpbios_scratchbuf;
	if (origlen > PNPBIOS_BUFSIZE - stacklen - 2)
		origlen = PNPBIOS_BUFSIZE - stacklen - 2;
	*(u_int16_t *)(pnpbios_scratchbuf) = origlen;

	res = pnpbioscall(((caddr_t)help) - pnpbios_scratchbuf);
	*len = *(u_int16_t *)pnpbios_scratchbuf;
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
pnpbios_id_to_string(pnpid, s)
	u_int32_t pnpid;
	char *s;
{
	static char hex[] = "0123456789ABCDEF";
	u_int8_t *id;
	
	id = (u_int8_t *)&pnpid;
	*s++ = 'A' + (id[0] >> 2) - 1;
	*s++ = 'A' + ((id[0] & 3) << 3) + (id[1] >> 5) - 1;
	*s++ = 'A' + (id[1] & 0x1f) - 1;
	*s++ = hex[id[2] >> 4];
	*s++ = hex[id[2] & 0x0f];
	*s++ = hex[id[3] >> 4];
	*s++ = hex[id[3] & 0x0f];
	*s = '\0';
}

static void
pnpbios_printres(r)
	struct pnpresources *r;
{
	struct pnp_mem *mem;
	struct pnp_io *io;
	struct pnp_irq *irq;
	struct pnp_dma *dma;
	int p = 0;

	mem = SIMPLEQ_FIRST(&r->mem);
	if (mem) {
		printf("mem");
		do {
			printf(" %x", mem->minbase);
			if (mem->len > 1)
				printf("-%x", mem->minbase + mem->len - 1);
		} while ((mem = SIMPLEQ_NEXT(mem, next)));
		p++;
	}
	io = SIMPLEQ_FIRST(&r->io);
	if (io) {
		if (p++)
			printf(", ");
		printf("io");
		do {
			printf(" %x", io->minbase);
			if (io->len > 1)
				printf("-%x", io->minbase + io->len - 1);
		} while ((io = SIMPLEQ_NEXT(io, next)));
	}
	irq = SIMPLEQ_FIRST(&r->irq);
	if (irq) {
		if (p++)
			printf(", ");
		printf("irq");
		do {
			printf(" %d", ffs(irq->mask) - 1);
		} while ((irq = SIMPLEQ_NEXT(irq, next)));
	}
	dma = SIMPLEQ_FIRST(&r->dma);
	if (dma) {
		if (p)
			printf(", ");
		printf("dma");
		do {
			printf(" %d", ffs(dma->mask) - 1);
		} while ((dma = SIMPLEQ_NEXT(dma, next)));
	}
}

static int
pnpbios_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (pnp)
		return (QUIET);

	printf(" index %d (%s", aa->idx, aa->primid);
	if (aa->resc->longname)
		printf(", %s", aa->resc->longname);
	if (aa->idstr != aa->primid)
		printf(", attached as %s", aa->idstr);
	printf(")");

	return (0);
}

void
pnpbios_print_devres(dev, aa)
	struct device *dev;
	struct pnpbiosdev_attach_args *aa;
{

	printf("%s: ", dev->dv_xname);
	pnpbios_printres(aa->resc);
	printf("\n");
}

static int
pnpbios_submatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (match->cf_loc[PNPBIOSCF_INDEX] != PNPBIOSCF_INDEX_DEFAULT &&
	    match->cf_loc[PNPBIOSCF_INDEX] != aa->idx)
		return (0);

	return ((*match->cf_attach->ca_match)(parent, match, aux));
}

static int
pnpbios_attachnode(sc, idx, buf, len, matchonly)
	struct pnpbios_softc *sc;
	int idx;
	const u_int8_t *buf;
	size_t len;
	int matchonly;
{
	struct pnpdevnode *dn;
	const u_int8_t *p;
	char idstr[8];
	struct pnpresources r, s;
	struct pnpbiosdev_attach_args aa;
	struct pnp_compatid *compatid;
	int res, i;

	dn = (struct pnpdevnode *)buf;
	pnpbios_id_to_string(dn->dn_product, idstr);
	p = (u_char *)(dn + 1);

	DPRINTF(("%s (%s): type 0x%02x subtype "
	    "0x%02x dpi 0x%02x attr 0x%04x:\n",
	    idstr, matchonly ? "static" : "dynamic", dn->dn_type,
	    dn->dn_subtype, dn->dn_dpi, dn->dn_attr));
	DPRINTF(("%s: allocated config scan:\n", idstr));
	res = pnp_scan(&p, len - 12, &r, 0);
	if (res < 0) {
		printf("error in config data\n");
		goto dump;
	}

	/*
	 * the following is consistency check only for now
	 */
	DPRINTF(("\tpossible config scan:\n"));
	res = pnp_scan(&p, len - (p - buf), &s, 0);
	if (res < 0) {
		printf("error in possible configuration\n");
		goto dump;
	}

	DPRINTF(("\tcompat id scan:\n"));
	res = pnp_scan(&p, len - (p - buf), &s, 0);
	if (res < 0) {
		printf("error in compatible ID\n");
		goto dump;
	}

	if (p != buf + len) {
		printf("%s: length mismatch in node %d: used %d of %d Bytes\n",
		       sc->sc_dev.dv_xname, idx, p - buf, len);
		if (p > buf + len) {
			/* XXX shouldn't happen - pnp_scan should catch it */
			goto dump;
		}
		/* Crappy BIOS: Buffer is not fully used. Be generous. */
	}

	if (r.nummem + r.numio + r.numirq + r.numdma == 0) {
		if (pnpbiosverbose) {
			printf("%s", idstr);
			if (r.longname)
				printf(", %s", r.longname);
			compatid = s.compatids;
			while (compatid) {
				printf(", %s", compatid->idstr);
				compatid = compatid->next;
			}
			printf(" at %s index %d disabled\n",
			    sc->sc_dev.dv_xname, idx);
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
	if (matchonly
	    ? config_search(pnpbios_submatch, (struct device *)sc, &aa) != NULL
	    : config_found_sm((struct device *)sc, &aa, pnpbios_print,
		pnpbios_submatch) != NULL)
			return -1;

	/* if no driver was found, try compatible IDs */
	compatid = s.compatids;
	while (compatid) {
		aa.idstr = compatid->idstr;
		if (matchonly
		    ? config_search(pnpbios_submatch, (struct device *)sc,
			&aa) != NULL
		    : config_found_sm((struct device *)sc, &aa, pnpbios_print,
			pnpbios_submatch) != NULL)
				return -1;
		compatid = compatid->next;
	}

	if (pnpbiosverbose) {
		printf("%s", idstr);
		if (r.longname)
			printf(", %s", r.longname);
		compatid = s.compatids;
		while (compatid) {
			printf(", %s", compatid->idstr);
			compatid = compatid->next;
		}
		printf(" (");
		pnpbios_printres(&r);
		printf(") at %s index %d ignored\n", sc->sc_dev.dv_xname, idx);
	}

	return 0;

	/* XXX should free resource lists */

dump:
	i = 0;
#ifdef PNPBIOSDEBUG
	/* print some useful info */
	if (len >= sizeof(*dn)) {
		printf("%s idx %d size %d type 0x%x:0x%x:0x%x attr 0x%x\n",
		    idstr, dn->dn_handle, dn->dn_size, dn->dn_type,
		    dn->dn_subtype, dn->dn_dpi, dn->dn_attr);
		i += sizeof(*dn);
	}
#endif
	for (; i < len; i++)
		printf(" %02x", buf[i]);
	printf("\n");
	return 0;
}

static int
pnp_scan(bufp, maxlen, r, in_depends)
	const u_int8_t **bufp;
	size_t maxlen;
	struct pnpresources *r;
	int in_depends;
{
	const void *start;
	const u_int8_t *p;
	struct pnp_mem *mem;
	int tag, type, len;
	char *idstr;
	int i;

	p = *bufp;

	bzero(r, sizeof(*r));
	SIMPLEQ_INIT(&r->mem);
	SIMPLEQ_INIT(&r->io);
	SIMPLEQ_INIT(&r->irq);
	SIMPLEQ_INIT(&r->dma);

	for (;;) {
		if (p >= *bufp + maxlen) {
			printf("pnp_scanresources: end of buffer\n");
			return (-1);
		}
		start = p;
		tag = *p;
		if (tag & ISAPNP_LARGE_TAG) {
			len = *(u_int16_t *)(p + 1);
			p += sizeof(struct pnplargeres) + len;

			switch (tag) {
			case ISAPNP_TAG_MEM_RANGE_DESC: {
				const struct pnpmem16rangeres *res = start;
				if (len != sizeof(*res) - 3) {
					printf("pnp_scan: bad mem desc\n");
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
					printf("ID in dep?\n");
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
					printf("pnp_scan: bad mem32 desc\n");
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
					printf("pnp_scan: bad mem32 desc\n");
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
				printf("pnp_scan: bad small resource\n");
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

				if (r->dependant_link) {
					printf("second dep?\n");
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
						printf("error in dependant "
						    "function\n");
						free(new, M_DEVBUF);
						return (-1);
					}
					last->dependant_link = new;
					last = new;
				} while (rv > 0);
				continue;
			}
			if (type == ISAPNP_TAG_DEP_END) {
				DPRINTF(("\ttag enddep\n"));
				if (!in_depends) {
					printf("tag %d end dep?\n", tag);
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
pnp_newirq(r, vres, len)
	struct pnpresources *r;
	const void *vres;
	size_t len;
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
pnp_newdma(r, vres, len)
	struct pnpresources *r;
	const void *vres;
	size_t len;
{
	const struct pnpdmares *res;
	struct pnp_dma *dma;

	res = vres;
	if (res->r_mask == 0) { /* disabled */
		DPRINTF(("\ttag dma zeroed\n"));
		return (0);
	}
	dma = malloc(sizeof(struct pnp_dma), M_DEVBUF, M_NOWAIT);
	dma->mask = res->r_mask;
	dma->flags = res->r_flags;
	SIMPLEQ_INSERT_TAIL(&r->dma, dma, next);
	r->numdma++;

	DPRINTF(("\ttag dma flags %02x mask %02x\n", dma->flags,dma->mask));

	return (0);
}

static int
pnp_newioport(r, vres, len)
	struct pnpresources *r;
	const void *vres;
	size_t len;
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
pnp_newfixedioport(r, vres, len)
	struct pnpresources *r;
	const void *vres;
	size_t len;
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
pnp_compatid(r, vres, len)
	struct pnpresources *r;
	const void *vres;
	size_t len;
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
pnp_debugdump(r, vres, len)
	struct pnpresources *r;
	const void *vres;
	size_t len;
{
	const u_int8_t *res;
	int type, i;

	if (res[0] & ISAPNP_LARGE_TAG) {
		type = res[0] & 0x7f;
		printf("\tTAG %02x len %04x %s", type, len, len ? "data" : "");
		i = 3;
	} else {
		type = (res[0] >> 3) & 0x0f;
		printf("\tTAG %02x len %02x %s", type, len, len ? "data" : "");
		i = 1;
	}
	for (; i < len; i++)
		printf(" %02x", res[i]);
	printf("\n");

	return (0);
}
#endif

int
pnpbios_io_map(pbt, resc, idx, tagp, hdlp)
	pnpbios_tag_t pbt;
	struct pnpresources *resc;
	int idx;
	bus_space_tag_t *tagp;
	bus_space_handle_t *hdlp;
{
	struct pnp_io *io;

	if (idx >= resc->numio)
		return (EINVAL);

	io = SIMPLEQ_FIRST(&resc->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	*tagp = I386_BUS_SPACE_IO;
	return (i386_memio_map(I386_BUS_SPACE_IO, io->minbase, io->len,
			       0, hdlp));
}

void
pnpbios_io_unmap(pbt, resc, idx, tag, hdl)
	pnpbios_tag_t pbt;
	struct pnpresources *resc;
	int idx;
	bus_space_tag_t tag;
	bus_space_handle_t hdl;
{
	struct pnp_io *io;

	if (idx >= resc->numio)
		return;

	io = SIMPLEQ_FIRST(&resc->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	i386_memio_unmap(tag, hdl, io->len);
}

int
pnpbios_getiobase(pbt, resc, idx, tagp, basep)
	pnpbios_tag_t pbt;
	struct pnpresources *resc;
	int idx;
	bus_space_tag_t *tagp;
	int *basep;
{
	struct pnp_io *io;

	if (idx >= resc->numio)
		return (EINVAL);

	io = SIMPLEQ_FIRST(&resc->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	if (tagp)
		*tagp = I386_BUS_SPACE_IO;
	if (basep)
		*basep = io->minbase;
	return (0);
}

int
pnpbios_getiosize(pbt, resc, idx, sizep)
        pnpbios_tag_t pbt;
        struct pnpresources *resc;
        int idx;
        int *sizep;
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
pnpbios_intr_establish(pbt, resc, idx, level, fcn, arg)
	pnpbios_tag_t pbt;
	struct pnpresources *resc;
	int idx, level;
	int (*fcn) __P((void *));
	void *arg;
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
pnpbios_getirqnum(pbt, resc, idx, irqp, istp)
	pnpbios_tag_t pbt;
	struct pnpresources *resc;
	int idx;
	int *irqp;
	int *istp;
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
pnpbios_getdmachan(pbt, resc, idx, chanp)
	pnpbios_tag_t pbt;
	struct pnpresources *resc;
	int idx;
	int *chanp;
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
pnpbios_create_event_thread(arg)
	void *arg;
{
	struct pnpbios_softc *sc;

	sc = arg;
	if (kthread_create1(pnpbios_event_thread, sc, &sc->sc_evthread,
	    "%s", sc->sc_dev.dv_xname))
		panic("pnpbios_create_event_thread");
}

static void
pnpbios_event_thread(arg)
	void *arg;
{
	struct pnpbios_softc *sc;
	u_int16_t event;
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

	config_pending_decr();

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
				    sc->sc_dev.dv_xname, rv);
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
				    sc->sc_dev.dv_xname);
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
			    sc->sc_dev.dv_xname);
#endif
			break;
		default:
#ifdef DIAGNOSTIC
			if (event & PNP_EID_OEM_DEFINED_BIT)
				printf("%s: vendor defined event 0x%04x\n",
				    sc->sc_dev.dv_xname, event);
			else
				printf("%s: unkown event 0x%04x\n",
				    sc->sc_dev.dv_xname, event);
#endif
			break;
		}
	}

	pnpbios_sendmessage(PNP_CM_PNP_OS_INACTIVE);
	kthread_exit(0);
}
#endif	/* PNPBIOSEVENTS */
