/*	$NetBSD: octeon_pip.c,v 1.14 2021/12/05 03:12:14 msaitoh Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_pip.c,v 1.14 2021/12/05 03:12:14 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <net/if.h>

#include <mips/locore.h>

#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_gmxreg.h>
#include <mips/cavium/dev/octeon_pipreg.h>
#include <mips/cavium/dev/octeon_pipvar.h>
#include <mips/cavium/include/iobusvar.h>

#include <dev/fdt/fdtvar.h>

static int	octpip_iobus_match(device_t, struct cfdata *, void *);
static void	octpip_iobus_attach(device_t, device_t, void *);

static int	octpip_fdt_match(device_t, struct cfdata *, void *);
static void	octpip_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(octpip_iobus, sizeof(struct octpip_softc),
    octpip_iobus_match, octpip_iobus_attach, NULL, NULL);

CFATTACH_DECL_NEW(octpip_fdt, sizeof(struct octpip_softc),
    octpip_fdt_match, octpip_fdt_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cavium,octeon-3860-pip" },
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry pip_compat_data[] = {
	{ .compat = "cavium,octeon-3860-pip-interface" },
	DEVICE_COMPAT_EOL
};

static int
octpip_iobus_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	return 1;
}

static void
octpip_iobus_attach(device_t parent, device_t self, void *aux)
{
	struct octpip_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	struct iobus_attach_args gmxaa;
	struct iobus_unit gmxiu;
	int i, ndevs;

	sc->sc_dev = self;

	aprint_normal("\n");

	/*
	 * XXX: In a non-FDT world, should allow for the configuration
	 * of multiple GMX devices.
	 */
	ndevs = 1;

	for (i = 0; i < ndevs; i++) {
		memcpy(&gmxaa, aa, sizeof(gmxaa));
		memset(&gmxiu, 0, sizeof(gmxiu));

		gmxaa.aa_name = "octgmx";
		gmxaa.aa_unitno = i;
		gmxaa.aa_unit = &gmxiu;
		gmxaa.aa_bust = aa->aa_bust;
		gmxaa.aa_dmat = aa->aa_dmat;

		if (MIPS_PRID_IMPL(mips_options.mips_cpu_id) == MIPS_CN68XX)
			gmxiu.addr = GMX_CN68XX_BASE_PORT(i, 0);
		else
			gmxiu.addr = GMX_BASE_PORT(i, 0);

		config_found(self, &gmxaa, NULL, CFARGS_NONE);
	}
}

static int
octpip_fdt_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
octpip_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct octpip_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct iobus_attach_args gmxaa;
	struct iobus_unit gmxiu;
	bus_addr_t intno;
	int child;

	sc->sc_dev = self;

	aprint_normal("\n");

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!of_compatible_match(child, pip_compat_data))
			continue;

		if (fdtbus_get_reg(child, 0, &intno, NULL) != 0) {
			aprint_error_dev(self, "couldn't get interface number for %s\n",
			    fdtbus_get_string(child, "name"));
			continue;
		}

		memset(&gmxaa, 0, sizeof(gmxaa));
		memset(&gmxiu, 0, sizeof(gmxiu));

		gmxaa.aa_name = "octgmx";
		gmxaa.aa_unitno = (int)intno;
		gmxaa.aa_unit = &gmxiu;
		gmxaa.aa_bust = faa->faa_bst;
		gmxaa.aa_dmat = faa->faa_dmat;

		if (MIPS_PRID_IMPL(mips_options.mips_cpu_id) == MIPS_CN68XX)
			gmxiu.addr = GMX_CN68XX_BASE_PORT(intno, 0);
		else
			gmxiu.addr = GMX_BASE_PORT(intno, 0);

		config_found(self, &gmxaa, NULL, CFARGS_NONE);

		/* XXX only one interface supported by octgmx */
		return;
	}
}

/* XXX */
void
octpip_init(struct octpip_attach_args *aa, struct octpip_softc **rsc)
{
	struct octpip_softc *sc;
	int status;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	sc->sc_port = aa->aa_port;
	sc->sc_regt = aa->aa_regt;
	sc->sc_tag_type = aa->aa_tag_type;
	sc->sc_receive_group = aa->aa_receive_group;
	sc->sc_ip_offset = aa->aa_ip_offset;

	status = bus_space_map(sc->sc_regt, PIP_BASE, PIP_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "pip register");

	*rsc = sc;
}

#define	_PIP_RD8(sc, off) \
	bus_space_read_8((sc)->sc_regt, (sc)->sc_regh, (off))
#define	_PIP_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_regt, (sc)->sc_regh, (off), (v))

int
octpip_port_config(struct octpip_softc *sc)
{
	uint64_t prt_cfg;
	uint64_t prt_tag;
	uint64_t ip_offset;

	/*
	 * Process the headers and place the IP header in the work queue
	 */
	prt_cfg = 0;
	if (MIPS_PRID_IMPL(mips_options.mips_cpu_id) == MIPS_CN50XX) {
		SET(prt_cfg, PIP_PRT_CFGN_LENERR_EN);
		SET(prt_cfg, PIP_PRT_CFGN_MAXERR_EN);
		SET(prt_cfg, PIP_PRT_CFGN_MINERR_EN);
	}
	/* RAWDRP=0; don't allow raw packet drop */
	/* TAGINC=0 */
	/* DYN_RS=0; disable dynamic short buffering */
	/* INST_HDR=0 */
	/* GRP_WAT=0 */
	SET(prt_cfg, __SHIFTIN(sc->sc_port, PIP_PRT_CFGN_QOS));
	/* QOS_WAT=0 */
	/* SPARE=0 */
	/* QOS_DIFF=0 */
	/* QOS_VLAN=0 */
	SET(prt_cfg, PIP_PRT_CFGN_CRC_EN);
	/* SKIP=0 */

	prt_tag = 0;
	SET(prt_tag, PIP_PRT_TAGN_INC_PRT);
	CLR(prt_tag, PIP_PRT_TAGN_IP6_DPRT);
	CLR(prt_tag, PIP_PRT_TAGN_IP4_DPRT);
	CLR(prt_tag, PIP_PRT_TAGN_IP6_SPRT);
	CLR(prt_tag, PIP_PRT_TAGN_IP4_SPRT);
	CLR(prt_tag, PIP_PRT_TAGN_IP6_NXTH);
	CLR(prt_tag, PIP_PRT_TAGN_IP4_PCTL);
	CLR(prt_tag, PIP_PRT_TAGN_IP6_DST);
	CLR(prt_tag, PIP_PRT_TAGN_IP4_SRC);
	CLR(prt_tag, PIP_PRT_TAGN_IP6_SRC);
	CLR(prt_tag, PIP_PRT_TAGN_IP4_DST);
	SET(prt_tag, __SHIFTIN(PIP_PRT_TAGN_TCP6_TAG_ORDERED, PIP_PRT_TAGN_TCP6_TAG));
	SET(prt_tag, __SHIFTIN(PIP_PRT_TAGN_TCP4_TAG_ORDERED, PIP_PRT_TAGN_TCP4_TAG));
	SET(prt_tag, __SHIFTIN(PIP_PRT_TAGN_IP6_TAG_ORDERED, PIP_PRT_TAGN_IP6_TAG));
	SET(prt_tag, __SHIFTIN(PIP_PRT_TAGN_IP4_TAG_ORDERED, PIP_PRT_TAGN_IP4_TAG));
	SET(prt_tag, __SHIFTIN(PIP_PRT_TAGN_NON_TAG_ORDERED, PIP_PRT_TAGN_NON_TAG));
	SET(prt_tag, sc->sc_receive_group & PIP_PRT_TAGN_GRP);

	ip_offset = 0;
	SET(ip_offset, (sc->sc_ip_offset / 8) & PIP_IP_OFFSET_MASK_OFFSET);

	_PIP_WR8(sc, PIP_PRT_CFG0_OFFSET + (8 * sc->sc_port), prt_cfg);
	_PIP_WR8(sc, PIP_PRT_TAG0_OFFSET + (8 * sc->sc_port), prt_tag);
	_PIP_WR8(sc, PIP_IP_OFFSET_OFFSET, ip_offset);

	return 0;
}

void
octpip_prt_cfg_enable(struct octpip_softc *sc, uint64_t prt_cfg, int enable)
{
	uint64_t tmp;

	tmp = _PIP_RD8(sc, PIP_PRT_CFG0_OFFSET + (8 * sc->sc_port));
	if (enable)
		tmp |= prt_cfg;
	else
		tmp &= ~prt_cfg;
	_PIP_WR8(sc, PIP_PRT_CFG0_OFFSET + (8 * sc->sc_port), tmp);
}

void
octpip_stats(struct octpip_softc *sc, struct ifnet *ifp, int gmx_port)
{
	uint64_t tmp, pkts;
	uint64_t pip_stat_ctl;

	if (sc == NULL || ifp == NULL)
		panic("%s: invalid argument. sc=%p, ifp=%p\n", __func__,
			sc, ifp);

	if (gmx_port < 0 || gmx_port > GMX_PORT_NUNITS) {
		printf("%s: invalid gmx_port %d\n", __func__, gmx_port);
		return;
	}

	pip_stat_ctl = _PIP_RD8(sc, PIP_STAT_CTL_OFFSET);
	_PIP_WR8(sc, PIP_STAT_CTL_OFFSET, pip_stat_ctl | PIP_STAT_CTL_RDCLR);
	tmp = _PIP_RD8(sc, PIP_STAT0_PRT_OFFSET(gmx_port));
	pkts = __SHIFTOUT(tmp, PIP_STAT0_PRTN_DRP_PKTS);
	if_statadd(ifp, if_iqdrops, pkts);

	_PIP_WR8(sc, PIP_STAT_CTL_OFFSET, pip_stat_ctl);
}
