/* $NetBSD: pnpbios.c,v 1.3 1999/11/15 21:50:50 drochner Exp $ */
/*
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <dev/isa/isareg.h>
#include <machine/isa_machdep.h>
#include <machine/segments.h>
#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <arch/i386/pnpbios/pnpbiosvar.h>

#include "opt_pnpbiosverbose.h"
#include "locators.h"

struct pnpbios_softc {
	struct device sc_dev;
	isa_chipset_tag_t sc_ic;
};

static caddr_t pnpbios_find __P((void));
static int pnpbios_match __P((struct device *, struct cfdata *, void *));
static void pnpbios_attach __P((struct device *, struct device *, void *));
static void pnpbios_printres __P((struct pnpresources *));
static int pnpbios_print __P((void *, const char *));
static int pnpbios_getnumnodes __P((int *, size_t *));
static int pnpbios_getnode __P((int, int *, unsigned char *, size_t));
static void eisaid_to_string __P((unsigned char *, char *));
static void pnpbios_attachnode __P((struct pnpbios_softc *, int,
				    unsigned char *, size_t));
static int pnp_scan __P((unsigned char **, size_t, struct pnpresources *, int));

static int pnpbios_submatch __P((struct device *, struct cfdata *, void *));

extern int pnpbioscall __P((int));

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
	unsigned char cksum;
	size_t structlen;

	for (p = (caddr_t)ISA_HOLE_VADDR(0xf0000);
	     p <= (caddr_t)ISA_HOLE_VADDR(0xffff0);
	     p += 16) {
		if (*(int *)p != PNPBIOS_SIGNATURE)
			continue;
		structlen = *(unsigned char *)(p + 5);
		if ((structlen < 0x21) ||
		    ((p + structlen - 1) > (caddr_t)ISA_HOLE_VADDR(0xfffff)))
			continue;

		cksum = 0;
		for (c = p; c < p + structlen; c++)
			cksum += *(unsigned char *)c;
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

static caddr_t mapit __P((u_long, u_long, int));

static caddr_t
mapit(addr, len, prot)
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
	caddr_t p;
	unsigned int codepbase, datapbase;
	caddr_t codeva, datava;
	extern char pnpbiostramp[], epnpbiostramp[];
	int res, num, i, size, idx;
	unsigned char *buf;

	pnpbios_softc = sc;
	sc->sc_ic = paa->paa_ic;

	isa_dmainit(sc->sc_ic, I386_BUS_SPACE_IO, &isa_bus_dma_tag, self);

	p = pnpbios_find();
	if (!p)
		panic("pnpbios_attach: disappeared");

	codepbase = *(unsigned int *)(p + 0x13);
	datapbase = *(unsigned int *)(p + 0x1d);
	pnpbios_entry = *(unsigned short *)(p + 0x11);

#ifdef PNPBIOSVERBOSE
	printf(": code %x, data %x, entry %x\n%s",
		codepbase, datapbase, pnpbios_entry, self->dv_xname);
#endif

	codeva = mapit(codepbase, 0x10000,
		VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	datava = mapit(datapbase, 0x10000,
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

	printf(": %d nodes, max len %d\n", num, size);
	buf = malloc(size, M_DEVBUF, M_NOWAIT);

	idx = 0;
	for (i = 0; i < num; i++) {
		int node = idx;
		res = pnpbios_getnode(1, &idx, buf, size);
		if (res) {
			printf("pnpbios_getnode: error %d\n", res);
			continue;
		}
		if (buf[2] != node)
			printf("node idx: called %d, got %d", node, buf[2]);
		pnpbios_attachnode(sc, node, buf, buf[0] + (buf[1] << 8));
	}
	if (idx != 0xff)
		printf("last idx=%x\n", idx);

	free(buf, M_DEVBUF);
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
	*--help = 0; /* GET_NUM_NODES */

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
	unsigned char *buf;
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
	*--help = 1; /* GET_DEVICE_NODE */

	*(short *)(pnpbios_scratchbuf + 0) = *idxp;

	res = pnpbioscall(((caddr_t)help) - pnpbios_scratchbuf);
	if (res)
		return (res);

	*idxp = *(short *)(pnpbios_scratchbuf + 0);
	bcopy(pnpbios_scratchbuf + 2, buf, len);
	return (0);
}

static void
eisaid_to_string(id, s)
	unsigned char *id;
	char *s;
{
	static char hex[] = "0123456789ABCDEF";
	
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

static void
pnpbios_attachnode(sc, idx, buf, len)
	struct pnpbios_softc *sc;
	int idx;
	unsigned char *buf;
	size_t len;
{
	char idstr[8];
	unsigned char *p;
	int res;
	struct pnpresources r, s;
	int i;
	struct pnpbiosdev_attach_args aa;
	struct pnp_compatid *compatid;

	eisaid_to_string(buf + 3, idstr);
	p = buf + 12;

	res = pnp_scan(&p, len - 12, &r, 0);
	if (res < 0) {
		printf("error in config data\n");
		goto dump;
	}

	/*
	 * the following is consistency check only for now
	 */
	res = pnp_scan(&p, len - (p - buf), &s, 0);
	if (res < 0) {
		printf("error in possible configuration\n");
		goto dump;
	}

	res = pnp_scan(&p, len - (p - buf), &s, 0);
	if (res < 0) {
		printf("error in compatible ID\n");
		goto dump;
	}

	if (p != buf + len) {
		printf("length mismatch\n");
		goto dump;
	}

	aa.pbt = 0; /* XXX placeholder */
	aa.idx = idx;
	aa.resc = &r;
	aa.ic = sc->sc_ic;
	aa.primid = idstr;

	/* first try the specific device ID */
	aa.idstr = idstr;
	if (config_found_sm((struct device *)sc, &aa, pnpbios_print,
	    pnpbios_submatch))
		return;

	/* if no driver was found, try compatible IDs */
	compatid = s.compatids;
	while (compatid) {
		aa.idstr = compatid->idstr;
		if (config_found_sm((struct device *)sc, &aa, pnpbios_print,
		    pnpbios_submatch))
			return;
		compatid = compatid->next;
	}

#ifdef PNPBIOSVERBOSE
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
#endif

	return;

	/* XXX should free ressource lists */

dump:
	for (i = 0; i < len; i++)
		printf(" %02x", buf[i]);
	printf("\n");
}

static int pnp_compatid __P((struct pnpresources *, unsigned char *, size_t));
static int pnp_newirq __P((struct pnpresources *, unsigned char *, size_t));
static int pnp_newdma __P((struct pnpresources *, unsigned char *, size_t));
static int pnp_newioport __P((struct pnpresources *, unsigned char *, size_t));

/*
 * small ressource types (beginning with 1)
 */
static struct{
	int (*handler) __P((struct pnpresources *, unsigned char *, size_t));
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
	{0, 3, 3}, /* fixed io descriptor */
	{0, -1, -1}, /* reserved */
	{0, -1, -1},
	{0, -1, -1},
	{0, -1, -1},
	{0, 1, 7}, /* vendor defined */
	{0, 1, 1} /* end */
};

#define NEXTBYTE(p) (*(p)++)

static int
pnp_scan(bufp, maxlen, r, in_depends)
	unsigned char **bufp;
	size_t maxlen;
	struct pnpresources *r;
	int in_depends;
{
	unsigned char *p = *bufp;
	int tag, type, len;
	char *idstr;
	int i;
	struct pnp_mem *mem;

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
		tag = NEXTBYTE(p);

		if (tag & 0x80) { /* long tag */
			type = tag & 0x7f;
			len = NEXTBYTE(p);
			len |= NEXTBYTE(p) << 8;

			switch (type) {
			case 0x01: /* memory descriptor */
				if (len != 9) {
					printf("pnp_scan: bad mem desc\n");
					return (-1);
				}

				mem = malloc(sizeof(struct pnp_mem),
					     M_DEVBUF, M_WAITOK);
				mem->flags = NEXTBYTE(p);
				mem->minbase = NEXTBYTE(p) << 8;
				mem->minbase |= NEXTBYTE(p) << 16;
				mem->maxbase = NEXTBYTE(p) << 8;
				mem->maxbase |= NEXTBYTE(p) << 16;
				mem->align = NEXTBYTE(p);
				mem->align |= NEXTBYTE(p) << 8;
				if (mem->align == 0)
					mem->align = 0x10000;
				mem->len = NEXTBYTE(p) << 8;
				mem->len |= NEXTBYTE(p) << 16;
				SIMPLEQ_INSERT_TAIL(&r->mem, mem, next);
				r->nummem++;
#ifdef PNPBIOSDEBUG
				if (mem->len == 0)
					printf("ZERO mem descriptor\n");
#endif
				break;
			case 0x02:
				if (in_depends)
					printf("ID in dep?\n");
				idstr = malloc(len + 1, M_DEVBUF, M_NOWAIT);
				for (i = 0; i < len; i++)
					idstr[i] = NEXTBYTE(p);
				idstr[len] = '\0';
				r->longname = idstr;
				break;
			case 0x06: /* 32bit fixed memory descriptor */
				if (len != 9) {
					printf("pnp_scan: bad mem32 desc\n");
					return (-1);
				}

				mem = malloc(sizeof(struct pnp_mem),
					     M_DEVBUF, M_WAITOK);
				mem->flags = NEXTBYTE(p);
				mem->minbase = NEXTBYTE(p);
				mem->minbase |= NEXTBYTE(p) << 8;
				mem->minbase |= NEXTBYTE(p) << 16;
				mem->minbase |= NEXTBYTE(p) << 24;
				mem->maxbase = mem->minbase;
				mem->align = 0;
				mem->len = NEXTBYTE(p);
				mem->len |= NEXTBYTE(p) << 8;
				mem->len |= NEXTBYTE(p) << 16;
				mem->len |= NEXTBYTE(p) << 24;
				SIMPLEQ_INSERT_TAIL(&r->mem, mem, next);
				r->nummem++;
#ifdef PNPBIOSDEBUG
				if (mem->len == 0)
					printf("ZERO mem descriptor\n");
#endif
				break;
			default:
				printf("ignoring long tag %x\n", type);
				while (len--)
					(void) NEXTBYTE(p);
			}
		} else {
			unsigned char tmpbuf[7];
			int i;

			type = (tag >> 3) & 0x0f;
			len = tag & 0x07;

			if (type == 0 ||
			    len < smallrescs[type - 1].minlen ||
			    len > smallrescs[type - 1].maxlen) {
				printf("pnp_scan: bad small resource\n");
				return (-1);
			}
			for (i = 0; i < len; i++)
				tmpbuf[i] = NEXTBYTE(p);

			if (type == 0x0f) { /* end mark */
				if (in_depends) {
					printf("end in dep?\n");
					return (-1);
				}
				break;
			}
			if (type == 0x06) { /* start dep */
				struct pnpresources *new, *last;
				int res;

				if (r->dependant_link) {
					printf("second dep?\n");
					return (-1);
				}

				if (in_depends) {
					*bufp = p;
					return (1);
				}

				last = r;
				do {
					new = malloc(sizeof(*new),
						     M_DEVBUF, M_NOWAIT);

					res = pnp_scan(&p, maxlen - (p - *bufp),
						       new, 1);
					if (res < 0) {
				printf("error in dependant function\n");
						free(new, M_DEVBUF);
						return (-1);
					}

					last->dependant_link = new;
					last = new;
				} while (res > 0);
				continue;
			}
			if (type == 0x07) { /* end dep */
				if (!in_depends) {
					printf("end dep?\n");
					return (-1);
				}
				break;
			}

			if (!smallrescs[type - 1].handler)
				printf("ignoring short tag %x\n", type);
			else
				if ((*smallrescs[type - 1].handler)(r, tmpbuf,
								    len))
					return (-1);
		}
	}
	*bufp = p;
	return (0);
}

static int
pnp_newirq(r, buf, len)
	struct pnpresources *r;
	unsigned char *buf;
	size_t len;
{
	struct pnp_irq *irq;

	irq = malloc(sizeof(struct pnp_irq), M_DEVBUF, M_NOWAIT);
	irq->mask = buf[0] | (buf[1] << 8);
	if (len > 2)
		irq->flags = buf[2];
	else
		irq->flags = 0x01;
	SIMPLEQ_INSERT_TAIL(&r->irq, irq, next);
	r->numirq++;
	return (0);
}

static int
pnp_newdma(r, buf, len)
	struct pnpresources *r;
	unsigned char *buf;
	size_t len;
{
	struct pnp_dma *dma;

	dma = malloc(sizeof(struct pnp_dma), M_DEVBUF, M_NOWAIT);
	dma->mask = buf[0];
	dma->flags = buf[1];
	SIMPLEQ_INSERT_TAIL(&r->dma, dma, next);
	r->numdma++;
	return (0);
}

static int
pnp_newioport(r, buf, len)
	struct pnpresources *r;
	unsigned char *buf;
	size_t len;
{
	struct pnp_io *io;

	io = malloc(sizeof(struct pnp_io), M_DEVBUF, M_NOWAIT);
	io->flags = buf[0];
	io->minbase = buf[1] | (buf[2] << 8);
	io->maxbase = buf[3] | (buf[4] << 8);
	io->align = buf[5];
	io->len = buf[6];
	SIMPLEQ_INSERT_TAIL(&r->io, io, next);
	r->numio++;
	return (0);
}

static int
pnp_compatid(r, buf, len)
	struct pnpresources *r;
	unsigned char *buf;
	size_t len;
{
	struct pnp_compatid *id;

	id = malloc(sizeof(*id), M_DEVBUF, M_NOWAIT);
	eisaid_to_string(buf, id->idstr);
	id->next = r->compatids;
	r->compatids = id;
	return (0);
}

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
pnpbios_getirqnum(pbt, resc, idx, irqp)
	pnpbios_tag_t pbt;
	struct pnpresources *resc;
	int idx;
	int *irqp;
{
	struct pnp_irq *irq;

	if (idx >= resc->numirq)
		return (EINVAL);

	irq = SIMPLEQ_FIRST(&resc->irq);
	while (idx--)
		irq = SIMPLEQ_NEXT(irq, next);

	*irqp = ffs(irq->mask) - 1;
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
