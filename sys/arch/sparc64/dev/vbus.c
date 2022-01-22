/*	$NetBSD: vbus.c,v 1.9 2022/01/22 11:49:17 thorpej Exp $	*/
/*	$OpenBSD: vbus.c,v 1.8 2015/09/27 11:29:20 kettenis Exp $	*/
/*
 * Copyright (c) 2008 Mark Kettenis
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/hypervisor.h>
#include <machine/openfirm.h>

#include <sparc64/dev/vbusvar.h>

#include <sparc64/dev/iommureg.h>

#include <dev/clock_subr.h>
extern todr_chip_handle_t todr_handle;

#ifdef DEBUG
#define VBUS_INTR             0x01
int vbus_debug = 0x00|VBUS_INTR;
#define DPRINTF(l, s)   do { if (vbus_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

struct vbus_softc {
	device_t		sc_dv;
	bus_space_tag_t		sc_bustag;
	bus_dma_tag_t		sc_dmatag;
};
int	vbus_cmp_cells(int *, int *, int *, int);
int	vbus_match(device_t, cfdata_t, void *);
void	vbus_attach(device_t, device_t, void *);
int	vbus_print(void *, const char *);

CFATTACH_DECL_NEW(vbus, sizeof(struct vbus_softc),
    vbus_match, vbus_attach, NULL, NULL);

void *vbus_intr_establish(bus_space_tag_t, int, int,
    int (*)(void *), void *, void (*)(void));
void	vbus_intr_ack(struct intrhand *);
bus_space_tag_t vbus_alloc_bus_tag(struct vbus_softc *, bus_space_tag_t);

int
vbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, "virtual-devices") == 0)
		return (1);

	return (0);
}

void
vbus_attach(device_t parent, device_t self, void *aux)
{
        struct vbus_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	int node;

	sc->sc_bustag = vbus_alloc_bus_tag(sc, ma->ma_bustag);
	sc->sc_dmatag = ma->ma_dmatag;
	printf("\n");

	devhandle_t selfh = device_handle(self);
	for (node = OF_child(ma->ma_node); node; node = OF_peer(node)) {
		struct vbus_attach_args va;
		char buf[32];

		bzero(&va, sizeof(va));
		va.va_node = node;
		if (OF_getprop(node, "name", buf, sizeof(buf)) <= 0)
			continue;
		va.va_name = buf;
		va.va_bustag = sc->sc_bustag;
		va.va_dmatag = sc->sc_dmatag;
		prom_getprop(node, "reg", sizeof(*va.va_reg),
			     &va.va_nreg, (void **)&va.va_reg);
		prom_getprop(node, "interrupts", sizeof(*va.va_intr),
			     &va.va_nintr, (void **)&va.va_intr);
		config_found(self, &va, vbus_print,
		    CFARGS(.devhandle = prom_node_to_devhandle(selfh,
							       va.va_node)));
	}

	struct vbus_attach_args va;
	bzero(&va, sizeof(va));
	va.va_name = "rtc";
	config_found(self, &va, vbus_print, CFARGS_NONE);

}

int
vbus_print(void *aux, const char *name)
{
	struct vbus_attach_args *va = aux;

	if (name)
		printf("\"%s\" at %s", va->va_name, name);
	return (UNCONF);
}

/*
 * Compare a sequence of cells with a mask, return 1 if they match and
 * 0 if they don't.
 */
int
vbus_cmp_cells(int *cell1, int *cell2, int *mask, int ncells)
{
	int i;

	for (i = 0; i < ncells; i++) {
		if (((cell1[i] ^ cell2[i]) & mask[i]) != 0)
			return (0);
	}
	return (1);
}

int
vbus_intr_map(int node, int ino, uint64_t *sysino)
{
	int *imap = NULL, nimap;
	int *reg = NULL, nreg;
	int *imap_mask;
	int parent;
	int address_cells, interrupt_cells;
	uint64_t devhandle;
	uint64_t devino;
	int len;
	int err;

	DPRINTF(VBUS_INTR, ("vbus_intr_map(): ino 0x%x\n", ino));

	parent = OF_parent(node);

	address_cells = prom_getpropint(parent, "#address-cells", 2);
	interrupt_cells = prom_getpropint(parent, "#interrupt-cells", 1);
	KASSERT(interrupt_cells == 1);

	len = OF_getproplen(parent, "interrupt-map-mask");
	if (len < (address_cells + interrupt_cells) * sizeof(int))
		return (-1);
	imap_mask = malloc(len, M_DEVBUF, M_WAITOK);
	if (OF_getprop(parent, "interrupt-map-mask", imap_mask, len) != len)
		goto out;

	if (prom_getprop(parent, "interrupt-map", sizeof(int), &nimap, (void **)&imap))
		panic("vbus: can't get interrupt-map");

	if (prom_getprop(node, "reg", sizeof(*reg), &nreg, (void **)&reg))
		panic("vbus: can't get reg");
	if (nreg < address_cells)
		goto out;

	while (nimap >= address_cells + interrupt_cells + 2) {
		if (vbus_cmp_cells(imap, reg, imap_mask, address_cells) &&
		    vbus_cmp_cells(&imap[address_cells], &ino,
		    &imap_mask[address_cells], interrupt_cells)) {
			node = imap[address_cells + interrupt_cells];
			devino = imap[address_cells + interrupt_cells + 1];

			free(reg, M_DEVBUF);
			reg = NULL;

			if (prom_getprop(node, "reg", sizeof(*reg), &nreg, (void **)&reg))
				panic("vbus: can't get reg");

			devhandle = reg[0] & 0x0fffffff;

			err = hv_intr_devino_to_sysino(devhandle, devino, sysino);
			if (err != H_EOK)
				goto out;

			KASSERT(*sysino == INTVEC(*sysino));
			free(imap_mask, M_DEVBUF);
			return (0);
		}
		imap += address_cells + interrupt_cells + 2;
		nimap -= address_cells + interrupt_cells + 2;
	}

out:
	free(imap_mask, M_DEVBUF);
	return (-1);
}

void *
vbus_intr_establish(bus_space_tag_t t, int ihandle, int level,
	int (*handler)(void *), void *arg, void (*fastvec)(void) /* ignored */)
{
	uint64_t sysino = INTVEC(ihandle);
	struct intrhand *ih;
	int ino;
	int err;

	DPRINTF(VBUS_INTR, ("vbus_intr_establish()\n"));

	ino = INTINO(ihandle);

	ih = intrhand_alloc();

	ih->ih_ivec = ihandle;
	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_pil = level;
	ih->ih_number = ino;
	ih->ih_pending = 0;

	intr_establish(ih->ih_pil, level != IPL_VM, ih);
	ih->ih_ack = vbus_intr_ack;

	err = hv_intr_settarget(sysino, cpus->ci_cpuid);
	if (err != H_EOK) {
		printf("hv_intr_settarget(%lu, %u) failed - err = %d\n", 
		       (long unsigned int)sysino, cpus->ci_cpuid, err);
		return (NULL);
	}

	/* Clear pending interrupts. */
	err = hv_intr_setstate(sysino, INTR_IDLE);
	if (err != H_EOK) {
	  printf("hv_intr_setstate(%lu, INTR_IDLE) failed - err = %d\n", 
		 (long unsigned int)sysino, err);
	  return (NULL);
	}

	err = hv_intr_setenabled(sysino, INTR_ENABLED);
	if (err != H_EOK) {
	  printf("hv_intr_setenabled(%lu) failed - err = %d\n", 
		 (long unsigned int)sysino, err);
	  return (NULL);
	}

	return (ih);
}

void
vbus_intr_ack(struct intrhand *ih)
{
	DPRINTF(VBUS_INTR, ("vbus_intr_ack()\n"));
	hv_intr_setstate(ih->ih_number, INTR_IDLE);
}

bus_space_tag_t
vbus_alloc_bus_tag(struct vbus_softc *sc, bus_space_tag_t parent)
{
	struct sparc_bus_space_tag *bt;

	bt = kmem_zalloc(sizeof(*bt), KM_SLEEP);
	bt->cookie = sc;
	bt->parent = parent;
	bt->sparc_bus_map = parent->sparc_bus_map;
	bt->sparc_intr_establish = vbus_intr_establish;

	return (bt);
}
