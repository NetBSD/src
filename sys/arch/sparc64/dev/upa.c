/*	$NetBSD: upa.c,v 1.24 2022/01/22 11:49:17 thorpej Exp $	*/
/*	$OpenBSD: upa.c,v 1.8 2008/01/17 22:53:18 kettenis Exp $	*/

/*
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Effort sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F30602-01-2-0537.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: upa.c,v 1.24 2022/01/22 11:49:17 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kmem.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>

struct upa_range {
	u_int64_t	ur_space;
	u_int64_t	ur_addr;
	u_int64_t	ur_len;
};

struct upa_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bt;
	bus_space_handle_t	sc_reg[3];
	struct upa_range	*sc_range;
	int			sc_node;
	int			sc_nrange;
	bus_space_tag_t		sc_cbt;
};

int	upa_match(device_t, cfdata_t, void *);
void	upa_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(upa, sizeof(struct upa_softc),
    upa_match, upa_attach, NULL, NULL);

int upa_print(void *, const char *);
bus_space_tag_t upa_alloc_bus_tag(struct upa_softc *);
int upa_bus_map(bus_space_tag_t, bus_addr_t,
    bus_size_t, int, vaddr_t, bus_space_handle_t *);
paddr_t upa_bus_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);

int
upa_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, "upa") == 0)
		return (1);

	return (0);
}

void
upa_attach(device_t parent, device_t self, void *aux)
{
	struct upa_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	int i, node;

	sc->sc_dev = self;
	sc->sc_bt = ma->ma_bustag;
	sc->sc_node = ma->ma_node;

	for (i = 0; i < 3; i++) {
		if (i >= ma->ma_nreg) {
			printf(": no register %d\n", i);
			return;
		}
		if (bus_space_map(sc->sc_bt, ma->ma_reg[i].ur_paddr,
		    ma->ma_reg[i].ur_len, 0, &sc->sc_reg[i])) {
			printf(": failed to map reg%d\n", i);
			return;
		}
	}

	if (prom_getprop(sc->sc_node, "ranges", sizeof(struct upa_range),
	    &sc->sc_nrange, (void **)&sc->sc_range))
		panic("upa: can't get ranges");

	printf("\n");

	sc->sc_cbt = upa_alloc_bus_tag(sc);

	devhandle_t selfh = device_handle(sc->sc_dev);
	for (node = OF_child(sc->sc_node); node; node = OF_peer(node)) {
		char buf[32];
		struct mainbus_attach_args map;

		bzero(&map, sizeof(map));
		if (OF_getprop(node, "device_type", buf, sizeof(buf)) <= 0)
			continue;
		if (prom_getprop(node, "reg", sizeof(*map.ma_reg),
		    &map.ma_nreg, (void **)&map.ma_reg) != 0)
			continue;
		if (OF_getprop(node, "name", buf, sizeof(buf)) <= 0)
			continue;
		map.ma_node = node;
		map.ma_name = buf;
		map.ma_bustag = sc->sc_cbt;
		map.ma_dmatag = ma->ma_dmatag;
		config_found(sc->sc_dev, &map, upa_print,
		    CFARGS(.devhandle = prom_node_to_devhandle(selfh, node)));
	}
}

int
upa_print(void *args, const char *name)
{
	struct mainbus_attach_args *ma = args;

	if (name)
		printf("\"%s\" at %s", ma->ma_name, name);
	return (UNCONF);
}

bus_space_tag_t
upa_alloc_bus_tag(struct upa_softc *sc)
{
	struct sparc_bus_space_tag *bt;

	bt = kmem_zalloc(sizeof(*bt), KM_SLEEP);
	*bt = *sc->sc_bt;
	bt->cookie = sc;
	bt->parent = sc->sc_bt;
	bt->sparc_bus_map = upa_bus_map;
	bt->sparc_bus_mmap = upa_bus_mmap;

	return bt;
}

int
upa_bus_map(bus_space_tag_t t, bus_addr_t offset,
    bus_size_t size, int flags, vaddr_t v, bus_space_handle_t *hp)
{
	struct upa_softc *sc = t->cookie;
	int i;

	if (t->parent == 0 || t->parent->sparc_bus_map == 0) {
		printf("\n__upa_bus_map: invalid parent");
		return (EINVAL);
	}

	t = t->parent;

	for (i = 0; i < sc->sc_nrange; i++) {
		if (offset < sc->sc_range[i].ur_space)
			continue;
		if (offset >= (sc->sc_range[i].ur_space +
		    sc->sc_range[i].ur_space))
			continue;
		break;
	}
	if (i == sc->sc_nrange)
		return (EINVAL);

	offset -= sc->sc_range[i].ur_space;
	offset += sc->sc_range[i].ur_addr;

	return ((*t->sparc_bus_map)(t, offset, size, flags, v, hp));
}

paddr_t
upa_bus_mmap(bus_space_tag_t t, bus_addr_t paddr, off_t off, int prot,
    int flags)
{
	struct upa_softc *sc = t->cookie;
	int i;

	if (t->parent == 0 || t->parent->sparc_bus_map == 0) {
		printf("\n__upa_bus_map: invalid parent");
		return (EINVAL);
	}

	t = t->parent;

	for (i = 0; i < sc->sc_nrange; i++) {
		if (paddr + off < sc->sc_range[i].ur_space)
			continue;
		if (paddr + off >= (sc->sc_range[i].ur_space +
		    sc->sc_range[i].ur_space))
			continue;
		break;
	}
	if (i == sc->sc_nrange)
		return (EINVAL);

	paddr -= sc->sc_range[i].ur_space;
	paddr += sc->sc_range[i].ur_addr;

	return ((*t->sparc_bus_mmap)(t, paddr, off, prot, flags));
}
