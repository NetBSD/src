/*	$NetBSD: cbus.c,v 1.8 2022/01/22 11:49:17 thorpej Exp $	*/
/*	$OpenBSD: cbus.c,v 1.15 2015/09/27 11:29:20 kettenis Exp $	*/
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
#include <sys/kmem.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/hypervisor.h>
#include <machine/openfirm.h>
#include <machine/mdesc.h>

#include <sparc64/dev/cbusvar.h>
#include <sparc64/dev/vbusvar.h>

#include <sparc64/dev/iommureg.h>

#ifdef DEBUG
#define CBUSDB_AC               0x01
#define CBUSDB_INTR             0x02
int cbus_debug = 0x00;
#define DPRINTF(l, s)   do { if (cbus_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

struct cbus_softc {
	device_t		sc_dv;
	bus_space_tag_t		sc_bustag;
	bus_dma_tag_t		sc_dmatag;

	uint64_t		sc_devhandle;

	/* Machine description. */
	int			sc_idx;
};

int	cbus_match(device_t, cfdata_t, void *);
void	cbus_attach(device_t, device_t, void *);
int	cbus_print(void *, const char *);

CFATTACH_DECL_NEW(cbus, sizeof(struct cbus_softc),
    cbus_match, cbus_attach, NULL, NULL);


void *cbus_intr_establish(bus_space_tag_t, int, int,
    int (*)(void *), void *, void (*)(void));
void cbus_intr_ack(struct intrhand *);
bus_space_tag_t cbus_alloc_bus_tag(struct cbus_softc *, bus_space_tag_t);

int cbus_get_channel_endpoint(struct cbus_softc *,
			      struct cbus_attach_args *);

int
cbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct vbus_attach_args *va = aux;

	if (strcmp(va->va_name, "channel-devices") == 0)
		return (1);

	return (0);
}

void
cbus_attach(device_t parent, device_t self, void *aux)
{
        struct cbus_softc *sc = device_private(self);
	struct vbus_attach_args *va = aux;
	int node;
	int reg;

	sc->sc_bustag = cbus_alloc_bus_tag(sc, va->va_bustag);
	sc->sc_dmatag = va->va_dmatag;

	if (OF_getprop(va->va_node, "reg", &reg, sizeof(reg)) != sizeof(reg))
		return;
	sc->sc_devhandle = reg;

	printf("\n");

	sc->sc_idx = mdesc_find(va->va_name, va->va_reg[0]);
	if (sc->sc_idx == -1) {
	  DPRINTF(CBUSDB_AC, ("cbus_attach() - no idx\n"));
	  return;
	}

	devhandle_t selfh = device_handle(self);
	for (node = OF_child(va->va_node); node; node = OF_peer(node)) {
		struct cbus_attach_args ca;
		char buf[32];

		bzero(&ca, sizeof(ca));
		ca.ca_node = node;
		if (OF_getprop(node, "name", buf, sizeof(buf)) <= 0)
			continue;
		DPRINTF(CBUSDB_AC, ("cbus_attach() - buf %s\n", buf));
		ca.ca_name = buf;
		ca.ca_bustag = sc->sc_bustag;
		ca.ca_dmatag = sc->sc_dmatag;
		prom_getprop(node, "reg", sizeof(*ca.ca_reg),
			     &ca.ca_nreg, (void **)&ca.ca_reg);
		int rc = cbus_get_channel_endpoint(sc, &ca);
		DPRINTF(CBUSDB_AC, ("cbus_attach() - cbus_get_channel_endpoint() %d\n", rc));
		if ( rc != 0) {
			continue;
		}

		config_found(self, &ca, cbus_print,
		    CFARGS(.devhandle = devhandle_from_of(selfh, ca.ca_node)));
	}
}

int
cbus_print(void *aux, const char *name)
{
	struct cbus_attach_args *ca = aux;
	DPRINTF(CBUSDB_AC, ("cbus_print() name %s\n", name));

	if (name)
		printf("\"%s\" at %s", ca->ca_name, name);
	if (ca->ca_id != -1)
	  printf(" chan 0x%llx", (long long unsigned int)ca->ca_id);
	return (UNCONF);
}

int
cbus_intr_setstate(bus_space_tag_t t, uint64_t devino, uint64_t state)
{
	struct cbus_softc *sc = t->cookie;
	uint64_t devhandle = sc->sc_devhandle;
	int err;

	err = hv_vintr_setstate(devhandle, devino, state);
	if (err != H_EOK)
		return (-1);

	return (0);
}

int
cbus_intr_setenabled(bus_space_tag_t t, uint64_t devino, uint64_t enabled)
{
	struct cbus_softc *sc = t->cookie;
	uint64_t devhandle = sc->sc_devhandle;
	int err;

	err = hv_vintr_setenabled(devhandle, devino, enabled);
	if (err != H_EOK)
		return (-1);

	return (0);
}

void *
cbus_intr_establish(bus_space_tag_t t, int ihandle, int level,
	int (*handler)(void *), void *arg, void (*fastvec)(void) /* ignored */)
{
	struct cbus_softc *sc = t->cookie;
	uint64_t devhandle = sc->sc_devhandle;
	uint64_t devino = ihandle;
	struct intrhand *ih;
	int ino;
	int err;

	ino = INTINO(ihandle);

	DPRINTF(CBUSDB_INTR, ("cbus_intr_establish(): ino 0x%x\n", ino));

	ih = intrhand_alloc();

	ih->ih_ivec = ihandle;
	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_pil = level;
	ih->ih_number = ino;
	ih->ih_bus = t;

	err = hv_vintr_setenabled(devhandle, devino, INTR_DISABLED);
	if (err != H_EOK) {
		printf("hv_vintr_setenabled: %d\n", err);
		return (NULL);
	}

	err = hv_vintr_setcookie(devhandle, devino, (vaddr_t)ih);
	if (err != H_EOK) {
		printf("hv_vintr_setcookie: %d\n", err);
		return (NULL);
	}

	ih->ih_ack = cbus_intr_ack;

	err = hv_vintr_settarget(devhandle, devino, cpus->ci_cpuid);
	if (err != H_EOK) {
		printf("hv_vintr_settarget: %d\n", err);
		return (NULL);
	}

	/* Clear pending interrupts. */
	err = hv_vintr_setstate(devhandle, devino, INTR_IDLE);
	if (err != H_EOK) {
		printf("hv_vintr_setstate: %d\n", err);
		return (NULL);
	}

	return (ih);
}

void
cbus_intr_ack(struct intrhand *ih)
{
	DPRINTF(CBUSDB_INTR, ("cbus_intr_ack()\n"));
	bus_space_tag_t t = ih->ih_bus;
	struct cbus_softc *sc = t->cookie;
	uint64_t devhandle = sc->sc_devhandle;
	uint64_t devino = ih->ih_number;

	hv_vintr_setstate(devhandle, devino, INTR_IDLE);
}

bus_space_tag_t
cbus_alloc_bus_tag(struct cbus_softc *sc, bus_space_tag_t parent)
{
	struct sparc_bus_space_tag *bt;

	bt = kmem_zalloc(sizeof(*bt), KM_SLEEP);
	bt->cookie = sc;
	bt->parent = parent;
	bt->sparc_bus_map = parent->sparc_bus_map;
	bt->sparc_intr_establish = cbus_intr_establish;

	return (bt);
}

int
cbus_get_channel_endpoint(struct cbus_softc *sc, struct cbus_attach_args *ca)
{
	struct md_header *hdr;
	struct md_element *elem;
	const char *name_blk;
	const char *str;
	int idx;
	int arc;

	idx = mdesc_find_child(sc->sc_idx, ca->ca_name, ca->ca_reg[0]);
	if (idx == -1) {
	  DPRINTF(CBUSDB_AC, ("cbus_get_channel_endpoint() - mdesc_find_child() failed\n"));
	  return (ENOENT);
	}
	DPRINTF(CBUSDB_AC, ("cbus_get_channel_endpoint() - idx %d\n", idx));

	hdr = (struct md_header *)mdesc;
	elem = (struct md_element *)(mdesc + sizeof(struct md_header));
	name_blk = (char *)mdesc + sizeof(struct md_header) + hdr->node_blk_sz;

	ca->ca_idx = idx;

	ca->ca_id = -1;
	ca->ca_tx_ino = -1;
	ca->ca_rx_ino = -1;

	if (strcmp(ca->ca_name, "disk") != 0 &&
	    strcmp(ca->ca_name, "network") != 0) {
	  DPRINTF(CBUSDB_AC, ("cbus_get_channel_endpoint() - neither disk nor network\n"));
	  return (0);
	}

	for (; elem[idx].tag != 'E'; idx++) {
		str = name_blk + elem[idx].name_offset;
		if (elem[idx].tag != 'a' || strcmp(str, "fwd") != 0)
			continue;

		arc = elem[idx].d.val;
		str = name_blk + elem[arc].name_offset;
		DPRINTF(CBUSDB_AC, ("cbus_get_channel_endpoint() - str %s\n", str));
		if (strcmp(str, "virtual-device-port") == 0) {
			idx = arc;
			DPRINTF(CBUSDB_AC, ("cbus_get_channel_endpoint() - idx %d\n", idx));
			continue;
		}

		if (strcmp(str, "channel-endpoint") == 0) {
			ca->ca_id = mdesc_get_prop_val(arc, "id");
			ca->ca_tx_ino = mdesc_get_prop_val(arc, "tx-ino");
			ca->ca_rx_ino = mdesc_get_prop_val(arc, "rx-ino");
			DPRINTF(CBUSDB_AC, ("cbus_get_channel_endpoint() "
					    "- tx-ino %lu rx-ino %lu\n", 
					    ca->ca_tx_ino, ca->ca_rx_ino));
			return (0);
		}
	}

	DPRINTF(CBUSDB_AC, ("cbus_get_channel_endpoint() - exit\n"));

	return (0);
}
