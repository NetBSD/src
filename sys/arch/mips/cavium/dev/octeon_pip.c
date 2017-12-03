/*	$NetBSD: octeon_pip.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_pip.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include "opt_octeon.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <net/if.h>
#include <mips/locore.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_pipreg.h>
#include <mips/cavium/dev/octeon_pipvar.h>

#ifdef OCTEON_ETH_DEBUG
struct octeon_pip_softc *__octeon_pip_softc;

void			octeon_pip_intr_evcnt_attach(struct octeon_pip_softc *);
void			octeon_pip_intr_rml(void *);

void			octeon_pip_dump(void);
void			octeon_pip_int_enable(struct octeon_pip_softc *, int);
#endif

/*
 * register definitions (for debug and statics)
 */
#define	_ENTRY(x)	{ #x, x##_BITS, x##_OFFSET }
#define	_ENTRY_0_3(x) \
	_ENTRY(x## 0), _ENTRY(x## 1), _ENTRY(x## 2), _ENTRY(x## 3)
#define	_ENTRY_0_7(x) \
	_ENTRY(x## 0), _ENTRY(x## 1), _ENTRY(x## 2), _ENTRY(x## 3), \
	_ENTRY(x## 4), _ENTRY(x## 5), _ENTRY(x## 6), _ENTRY(x## 7)
#define	_ENTRY_0_1_2_32(x) \
	_ENTRY(x## 0), _ENTRY(x## 1), _ENTRY(x## 2), _ENTRY(x##32)

struct octeon_pip_dump_reg_ {
	const char *name;
	const char *format;
	size_t	offset;
};

static const struct octeon_pip_dump_reg_ octeon_pip_dump_stats_[] = {
/* PIP_QOS_DIFF[0-63] */
	_ENTRY_0_1_2_32	(PIP_STAT0_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT1_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT2_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT3_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT4_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT5_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT6_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT7_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT8_PRT),
	_ENTRY_0_1_2_32	(PIP_STAT9_PRT),
/* PIP_TAG_INC[0-63] */
	_ENTRY_0_1_2_32	(PIP_STAT_INB_PKTS),
	_ENTRY_0_1_2_32	(PIP_STAT_INB_OCTS),
	_ENTRY_0_1_2_32	(PIP_STAT_INB_ERRS),
};

static const struct octeon_pip_dump_reg_ octeon_pip_dump_regs_[] = {
	_ENTRY		(PIP_BIST_STATUS),
	_ENTRY		(PIP_INT_REG),
	_ENTRY		(PIP_INT_EN),
	_ENTRY		(PIP_STAT_CTL),
	_ENTRY		(PIP_GBL_CTL),
	_ENTRY		(PIP_GBL_CFG),
	_ENTRY		(PIP_SOFT_RST),
	_ENTRY		(PIP_IP_OFFSET),
	_ENTRY		(PIP_TAG_SECRET),
	_ENTRY		(PIP_TAG_MASK),
	_ENTRY_0_3	(PIP_DEC_IPSEC),
	_ENTRY		(PIP_RAW_WORD),
	_ENTRY_0_7	(PIP_QOS_VLAN),
	_ENTRY_0_3	(PIP_QOS_WATCH),
	_ENTRY_0_1_2_32	(PIP_PRT_CFG),
	_ENTRY_0_1_2_32	(PIP_PRT_TAG),
};
#undef	_ENTRY
#undef	_ENTRY_0_3
#undef	_ENTRY_0_7
#undef	_ENTRY_0_1_2_32

/* XXX */
void
octeon_pip_init(struct octeon_pip_attach_args *aa,
    struct octeon_pip_softc **rsc)
{
	struct octeon_pip_softc *sc;
	int status;

	sc = malloc(sizeof(*sc), M_DEVBUF, M_WAITOK | M_ZERO);
	if (sc == NULL)
		panic("can't allocate memory: %s", __func__);

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

#ifdef OCTEON_ETH_DEBUG
	octeon_pip_int_enable(sc, 1);
	octeon_pip_intr_evcnt_attach(sc);
	__octeon_pip_softc = sc;
	printf("PIP Code initialized.\n");
#endif
}

#define	_PIP_RD8(sc, off) \
	bus_space_read_8((sc)->sc_regt, (sc)->sc_regh, (off))
#define	_PIP_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_regt, (sc)->sc_regh, (off), (v))

int
octeon_pip_port_config(struct octeon_pip_softc *sc)
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
	SET(prt_cfg, PIP_PRT_CFGN_DYN_RS);
	/* INST_HDR=0 */
	/* GRP_WAT=0 */
	SET(prt_cfg, (sc->sc_port << 24) & PIP_PRT_CFGN_QOS);
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
	SET(prt_tag, PIP_PRT_TAGN_TCP6_TAG_ORDERED);
	SET(prt_tag, PIP_PRT_TAGN_TCP4_TAG_ORDERED);
	SET(prt_tag, PIP_PRT_TAGN_IP6_TAG_ORDERED);
	SET(prt_tag, PIP_PRT_TAGN_IP4_TAG_ORDERED);
	SET(prt_tag, PIP_PRT_TAGN_NON_TAG_ORDERED);
	SET(prt_tag, sc->sc_receive_group & PIP_PRT_TAGN_GRP);

	ip_offset = 0;
	SET(ip_offset, (sc->sc_ip_offset / 8) & PIP_IP_OFFSET_MASK_OFFSET);

	_PIP_WR8(sc, PIP_PRT_CFG0_OFFSET + (8 * sc->sc_port), prt_cfg);
	_PIP_WR8(sc, PIP_PRT_TAG0_OFFSET + (8 * sc->sc_port), prt_tag);
	_PIP_WR8(sc, PIP_IP_OFFSET_OFFSET, ip_offset);

	return 0;
}

void
octeon_pip_prt_cfg_enable(struct octeon_pip_softc *sc, uint64_t prt_cfg,
    int enable)
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
octeon_pip_stats(struct octeon_pip_softc *sc, struct ifnet *ifp, int gmx_port)
{
	const struct octeon_pip_dump_reg_ *reg;
	uint64_t tmp, pkts;
	uint64_t pip_stat_ctl;

	if (sc == NULL || ifp == NULL)
		panic("%s: invalid argument. sc=%p, ifp=%p\n", __func__,
			sc, ifp);

	if (gmx_port < 0 || gmx_port > 2) {
		printf("%s: invalid gmx_port %d\n", __func__, gmx_port);
		return;
	}

	pip_stat_ctl = _PIP_RD8(sc, PIP_STAT_CTL_OFFSET);
	_PIP_WR8(sc, PIP_STAT_CTL_OFFSET, pip_stat_ctl | PIP_STAT_CTL_RDCLR);
	reg = &octeon_pip_dump_stats_[gmx_port];
	tmp = _PIP_RD8(sc, reg->offset);
	pkts = (tmp & 0xffffffff00000000ULL) >> 32;
	ifp->if_iqdrops += pkts;

	_PIP_WR8(sc, PIP_STAT_CTL_OFFSET, pip_stat_ctl);
}


#ifdef OCTEON_ETH_DEBUG
int			octeon_pip_intr_rml_verbose;
struct evcnt		octeon_pip_intr_evcnt;

static const struct octeon_evcnt_entry octeon_pip_intr_evcnt_entries[] = {
#define	_ENTRY(name, type, parent, descr) \
	OCTEON_EVCNT_ENTRY(struct octeon_pip_softc, name, type, parent, descr)
	_ENTRY(pipbeperr,	MISC, NULL, "pip parity error backend"),
	_ENTRY(pipfeperr,	MISC, NULL, "pip parity error frontend"),
	_ENTRY(pipskprunt,	MISC, NULL, "pip skiper"),
	_ENTRY(pipbadtag,	MISC, NULL, "pip bad tag"),
	_ENTRY(pipprtnxa,	MISC, NULL, "pip nonexistent port"),
	_ENTRY(pippktdrp,	MISC, NULL, "pip qos drop"),
#undef	_ENTRY
};

void
octeon_pip_intr_evcnt_attach(struct octeon_pip_softc *sc)
{
	OCTEON_EVCNT_ATTACH_EVCNTS(sc, octeon_pip_intr_evcnt_entries, "pip0");
}

void
octeon_pip_intr_rml(void *arg)
{
	struct octeon_pip_softc *sc;
	uint64_t reg;

	octeon_pip_intr_evcnt.ev_count++;
	sc = __octeon_pip_softc;
	KASSERT(sc != NULL);
	reg = octeon_pip_int_summary(sc);
	if (octeon_pip_intr_rml_verbose)
		printf("%s: PIP_INT_REG=0x%016" PRIx64 "\n", __func__, reg);
	if (reg & PIP_INT_REG_BEPERR)
		OCTEON_EVCNT_INC(sc, pipbeperr);
	if (reg & PIP_INT_REG_FEPERR)
		OCTEON_EVCNT_INC(sc, pipfeperr);
	if (reg & PIP_INT_REG_SKPRUNT)
		OCTEON_EVCNT_INC(sc, pipskprunt);
	if (reg & PIP_INT_REG_BADTAG)
		OCTEON_EVCNT_INC(sc, pipbadtag);
	if (reg & PIP_INT_REG_PRTNXA)
		OCTEON_EVCNT_INC(sc, pipprtnxa);
	if (reg & PIP_INT_REG_PKTDRP)
		OCTEON_EVCNT_INC(sc, pippktdrp);
}

void		octeon_pip_dump_regs(void);
void		octeon_pip_dump_stats(void);

void
octeon_pip_dump(void)
{
	octeon_pip_dump_regs();
	octeon_pip_dump_stats();
}

void
octeon_pip_dump_regs(void)
{
	struct octeon_pip_softc *sc = __octeon_pip_softc;
	const struct octeon_pip_dump_reg_ *reg;
	uint64_t tmp;
	char buf[512];
	int i;

	for (i = 0; i < (int)__arraycount(octeon_pip_dump_regs_); i++) {
		reg = &octeon_pip_dump_regs_[i];
		tmp = _PIP_RD8(sc, reg->offset);
		if (reg->format == NULL) {
			snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		} else {
			snprintb(buf, sizeof(buf), reg->format, tmp);
		}
		printf("\t%-24s: %s\n", reg->name, buf);
	}
}

void
octeon_pip_dump_stats(void)
{
	struct octeon_pip_softc *sc = __octeon_pip_softc;
	const struct octeon_pip_dump_reg_ *reg;
	uint64_t tmp;
	char buf[512];
	int i;
	uint64_t pip_stat_ctl;

	pip_stat_ctl = _PIP_RD8(sc, PIP_STAT_CTL_OFFSET);
	_PIP_WR8(sc, PIP_STAT_CTL_OFFSET, pip_stat_ctl & ~PIP_STAT_CTL_RDCLR);
	for (i = 0; i < (int)__arraycount(octeon_pip_dump_stats_); i++) {
		reg = &octeon_pip_dump_stats_[i];
		tmp = _PIP_RD8(sc, reg->offset);
		if (reg->format == NULL) {
			snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		} else {
			snprintb(buf, sizeof(buf), reg->format, tmp);
		}
		printf("\t%-24s: %s\n", reg->name, buf);
	}
	printf("\t%-24s:\n", "PIP_QOS_DIFF[0-63]");
	for (i = 0; i < 64; i++) {
		tmp = _PIP_RD8(sc, PIP_QOS_DIFF0_OFFSET + sizeof(uint64_t) * i);
		snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		printf("%s\t%s%s",
		    ((i % 4) == 0) ? "\t" : "",
		    buf,
		    ((i % 4) == 3) ? "\n" : "");
	}
	printf("\t%-24s:\n", "PIP_TAG_INC[0-63]");
	for (i = 0; i < 64; i++) {
		tmp = _PIP_RD8(sc, PIP_TAG_INC0_OFFSET + sizeof(uint64_t) * i);
		snprintf(buf, sizeof(buf), "%16" PRIx64, tmp);
		printf("%s\t%s%s",
		    ((i % 4) == 0) ? "\t" : "",
		    buf,
		    ((i % 4) == 3) ? "\n" : "");
	}
	_PIP_WR8(sc, PIP_STAT_CTL_OFFSET, pip_stat_ctl);
}

void
octeon_pip_int_enable(struct octeon_pip_softc *sc, int enable)
{
	uint64_t pip_int_xxx = 0;

	SET(pip_int_xxx,
	    PIP_INT_EN_BEPERR |
	    PIP_INT_EN_FEPERR |
	    PIP_INT_EN_SKPRUNT |
	    PIP_INT_EN_BADTAG |
	    PIP_INT_EN_PRTNXA |
	    PIP_INT_EN_PKTDRP);
	_PIP_WR8(sc, PIP_INT_REG_OFFSET, pip_int_xxx);
	_PIP_WR8(sc, PIP_INT_EN_OFFSET, enable ? pip_int_xxx : 0);
}
uint64_t
octeon_pip_int_summary(struct octeon_pip_softc *sc)
{
	uint64_t summary;

	summary = _PIP_RD8(sc, PIP_INT_REG_OFFSET);
	_PIP_WR8(sc, PIP_INT_REG_OFFSET, summary);
	return summary;
}
#endif /* OCTEON_ETH_DEBUG */
