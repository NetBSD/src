/*	$NetBSD: pq3pci.c,v 1.16.2.1 2014/08/10 06:54:05 tls Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define	PCI_PRIVATE
#define	GLOBAL_PRIVATE
#define	__INTR_PRIVATE

#include "opt_mpc85xx.h"
#include "opt_pci.h"
#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pq3pci.c,v 1.16.2.1 2014/08/10 06:54:05 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/extent.h>
#include <sys/bitops.h>
#include <sys/kmem.h>
#include <sys/malloc.h>	/* for extent */

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcidevs.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/spr.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/openpicreg.h>

#define	PORDEVSR_MPC8536_TRUTH_ENCODE(inst, field, value, result) \
    TRUTH_ENCODE(SVR_MPC8536v1, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##MPC8536##_##value, PORDEVSR_##field), result)
#define	PORDEVSR_MPC8544_TRUTH_ENCODE(inst, field, value, result) \
    TRUTH_ENCODE(SVR_MPC8544v1, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##MPC8544##_##value, PORDEVSR_##field), result)
#define	PORDEVSR_MPC8548_TRUTH_ENCODE(inst, field, value, result) \
    TRUTH_ENCODE(SVR_MPC8548v1, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##MPC8548##_##value, PORDEVSR_##field), result)
#define	PORDEVSR_MPC8555_TRUTH_ENCODE(inst, field, value, result) \
    TRUTH_ENCODE(SVR_MPC8555v1, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##MPC8555##_##value, PORDEVSR_##field), result)
#define	PORDEVSR_MPC8572_TRUTH_ENCODE(inst, field, value, result) \
    TRUTH_ENCODE(SVR_MPC8572v1, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##MPC8572##_##value, PORDEVSR_##field), result)
#define	PORDEVSR_P20x0_TRUTH_ENCODE(inst, field, value, result) \
    TRUTH_ENCODE(SVR_P2020v2, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##P20x0##_##value, PORDEVSR_##field), result), \
    TRUTH_ENCODE(SVR_P2010v2, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##P20x0##_##value, PORDEVSR_##field), result)
#define	PORDEVSR_P1025_TRUTH_ENCODE(inst, field, value, result) \
    TRUTH_ENCODE(SVR_P1025v1, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##P20x0##_##value, PORDEVSR_##field), result), \
    TRUTH_ENCODE(SVR_P1016v1, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##P20x0##_##value, PORDEVSR_##field), result)

#define	PORDEVSR_TRUTH_ENCODE(svr, inst, field, value, result) \
    TRUTH_ENCODE(svr, inst, PORDEVSR_##field, \
	__SHIFTIN(field##_##value, PORDEVSR_##field), result)

const struct e500_truthtab pq3pci_pcie_lanes[] = {
#ifdef MPC8548
    PORDEVSR_MPC8548_TRUTH_ENCODE(0, IOSEL, SRIO2500_PCIE1_X4, 4),
    PORDEVSR_MPC8548_TRUTH_ENCODE(0, IOSEL, SRIO1250_PCIE1_X4, 4),
    PORDEVSR_MPC8548_TRUTH_ENCODE(0, IOSEL, PCIE1_X8, 8),
#endif

#ifdef MPC8544
    PORDEVSR_MPC8544_TRUTH_ENCODE(1, IOSEL, PCIE1_ON, 4),
    PORDEVSR_MPC8544_TRUTH_ENCODE(1, IOSEL, PCIE1_SGMII_ON, 4),
    PORDEVSR_MPC8544_TRUTH_ENCODE(1, IOSEL, PCIE12_ON, 4),
    PORDEVSR_MPC8544_TRUTH_ENCODE(1, IOSEL, PCIE12_SGMII_ON, 4),
    PORDEVSR_MPC8544_TRUTH_ENCODE(1, IOSEL, PCIE123_ON, 4),
    PORDEVSR_MPC8544_TRUTH_ENCODE(1, IOSEL, PCIE123_SGMII_ON, 4),

    PORDEVSR_MPC8544_TRUTH_ENCODE(2, IOSEL, PCIE12_ON, 4),
    PORDEVSR_MPC8544_TRUTH_ENCODE(2, IOSEL, PCIE12_SGMII_ON, 4),
    PORDEVSR_MPC8544_TRUTH_ENCODE(2, IOSEL, PCIE123_ON, 4),
    PORDEVSR_MPC8544_TRUTH_ENCODE(2, IOSEL, PCIE123_SGMII_ON, 4),

    PORDEVSR_MPC8544_TRUTH_ENCODE(3, IOSEL, PCIE123_ON, 1),
    PORDEVSR_MPC8544_TRUTH_ENCODE(3, IOSEL, PCIE123_SGMII_ON, 1),
#endif

#ifdef MPC8536
    PORDEVSR_MPC8536_TRUTH_ENCODE(1, IOSEL, PCIE1_X4, 4),
    PORDEVSR_MPC8536_TRUTH_ENCODE(1, IOSEL, PCIE1_X8, 8),
    PORDEVSR_MPC8536_TRUTH_ENCODE(1, IOSEL, PCIE12_X4, 4),
    PORDEVSR_MPC8536_TRUTH_ENCODE(1, IOSEL, PCIE1_X4_PCI23_X2, 4),

    PORDEVSR_MPC8536_TRUTH_ENCODE(2, IOSEL, PCIE12_X4, 4),
    PORDEVSR_MPC8536_TRUTH_ENCODE(2, IOSEL, PCIE1_X4_PCI23_X2, 2),

    PORDEVSR_MPC8536_TRUTH_ENCODE(3, IOSEL, PCIE1_X4_PCI23_X2, 2),
#endif

#ifdef MPC8572
    PORDEVSR_MPC8572_TRUTH_ENCODE(1, IOSEL, SRIO2500_PCIE1_X4, 4),
    PORDEVSR_MPC8572_TRUTH_ENCODE(1, IOSEL, SRIO1250_PCIE1_X4, 4),
    PORDEVSR_MPC8572_TRUTH_ENCODE(1, IOSEL, PCIE1_X4, 4),
    PORDEVSR_MPC8572_TRUTH_ENCODE(1, IOSEL, PCIE12_X4, 4),
    PORDEVSR_MPC8572_TRUTH_ENCODE(1, IOSEL, PCIE1_X4_23_X2, 4),
    PORDEVSR_MPC8572_TRUTH_ENCODE(1, IOSEL, PCIE1_X8, 8),

    PORDEVSR_MPC8572_TRUTH_ENCODE(2, IOSEL, PCIE12_X4, 4),
    PORDEVSR_MPC8572_TRUTH_ENCODE(2, IOSEL, PCIE1_X4_23_X2, 2),

    PORDEVSR_MPC8572_TRUTH_ENCODE(3, IOSEL, PCIE1_X4_23_X2, 2),
#endif

#ifdef P2020
    PORDEVSR_P20x0_TRUTH_ENCODE(1, IOSEL, PCIE1_X1, 1),
    PORDEVSR_P20x0_TRUTH_ENCODE(1, IOSEL, PCIE12_X1_3_X2, 1),
    PORDEVSR_P20x0_TRUTH_ENCODE(1, IOSEL, PCIE13_X2, 2),
    PORDEVSR_P20x0_TRUTH_ENCODE(1, IOSEL, PCIE1_X4, 4),
    PORDEVSR_P20x0_TRUTH_ENCODE(1, IOSEL, PCIE1_X1_SRIO2500_1X, 1),
    PORDEVSR_P20x0_TRUTH_ENCODE(1, IOSEL, PCIE12_X1_SGMII23, 1),
    PORDEVSR_P20x0_TRUTH_ENCODE(1, IOSEL, PCIE1_X2_SGMII23, 2),

    PORDEVSR_P20x0_TRUTH_ENCODE(2, IOSEL, PCIE12_X1_3_X2, 1),
    PORDEVSR_P20x0_TRUTH_ENCODE(2, IOSEL, PCIE12_X1_SGMII23, 1),

    PORDEVSR_P20x0_TRUTH_ENCODE(3, IOSEL, PCIE12_X1_3_X2, 2),
    PORDEVSR_P20x0_TRUTH_ENCODE(3, IOSEL, PCIE13_X2, 2),
#endif

#ifdef P1025
    PORDEVSR_P1025_TRUTH_ENCODE(1, IOSEL, PCIE1_X1, 1),
    PORDEVSR_P1025_TRUTH_ENCODE(1, IOSEL, PCIE1_X4, 4),
    PORDEVSR_P1025_TRUTH_ENCODE(1, IOSEL, PCIE12_X1_SGMII23, 1),
    PORDEVSR_P1025_TRUTH_ENCODE(1, IOSEL, PCIE1_X2_SGMII23, 2),

    PORDEVSR_P1025_TRUTH_ENCODE(2, IOSEL, PCIE12_X1_SGMII23, 1),
#endif
};

static const struct e500_truthtab pq3pci_pci_pcix[] = {
#ifdef MPC8548
    PORDEVSR_TRUTH_ENCODE(SVR_MPC8548v1, 1, PCI1, PCIX, 1),
#endif
};

static const struct e500_truthtab pq3pci_pci_pci32[] = {
#ifdef MPC8548
    PORDEVSR_TRUTH_ENCODE(SVR_MPC8548v1, 1, PCI32, FALSE, 64),
    PORDEVSR_TRUTH_ENCODE(SVR_MPC8548v1, 1, PCI32, TRUE, 32),
#endif

#ifdef MPC8555
    PORDEVSR_TRUTH_ENCODE(SVR_MPC8555v1, 0, PCI32, FALSE, 64),
    PORDEVSR_TRUTH_ENCODE(SVR_MPC8555v1, 0, PCI32, TRUE, 32),
#endif
};

struct pq3pci_bst {
	struct powerpc_bus_space bs_tag;
	uint8_t bs_numwin;
	bus_addr_t bs_base[3];
	bus_addr_t bs_offset[3];
	bus_addr_t bs_limit[3];
	char bs_name[16];
	char bs_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)] __aligned(8);
};

typedef enum { IH_NONE, IH_INTX, IH_MSI, IH_MSIX } pq3pci_intr_class_t;

struct pq3pci_genihand {
	pq3pci_intr_class_t ih_class;
	int (*ih_func)(void *);
	void *ih_arg;
	struct pq3pci_softc *ih_sc;
};

struct pq3pci_intrhand {
	struct pq3pci_genihand pih_ih;
	SIMPLEQ_ENTRY(pq3pci_intrhand) pih_link;
	int pih_ipl;
	struct pq3pci_intrsource *pih_source;
	uint64_t pih_count;
};

struct pq3pci_callhand {
	struct pq3pci_genihand pch_ih;
	struct callout pch_callout;
	int pch_ipl;
};

#define	PIH_MAKE(irq, ist, nmsi) (((nmsi) << 20) | ((irq) << 8) | (ist))
#define	PIH_IST(pih)		(((pih) >> 0) & 0xff)
#define	PIH_IRQ(pih)		(((pih) >> 8) & 0xfff)
#define	PIH_NMSI(pih)		(((pih) >> 20) & 0xff)

struct pq3pci_intrsource {
	SIMPLEQ_ENTRY(pq3pci_intrsource) pis_link;
	SIMPLEQ_HEAD(,pq3pci_intrhand) pis_ihands;
	struct evcnt pis_ev;
	struct evcnt pis_ev_spurious;
	kmutex_t *pis_lock;
	pci_intr_handle_t pis_handle;
	void *pis_ih;
};

struct pq3pci_msihand {
	struct pq3pci_genihand msih_ih;
	struct pq3pci_msigroup *msih_group;
	struct evcnt msih_ev;
	struct evcnt msih_ev_spurious;
	pcitag_t msih_tag;
	int msih_msioff;
};

struct pq3pci_msigroup {
	kmutex_t *msig_lock;
	void *msig_ih;
	uint32_t msig_free_mask;
	int msig_ipl;
	u_int msig_group;
	bus_size_t msig_msir;
	struct pq3pci_msihand msig_ihands[32];
};

struct pq3pci_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	void *sc_ih;
	bool sc_pcie;
	struct genppc_pci_chipset sc_pc;
	struct pq3pci_bst sc_pci_io_bst;
	struct pq3pci_bst sc_pci_mem_bst;
	u_int sc_pba_flags;
	kmutex_t *sc_conf_lock;
	kmutex_t *sc_intr_lock;
	struct evcnt sc_ev_spurious;
	prop_dictionary_t sc_intrmap;
	uint32_t sc_intrmask;
};

static int pq3pci_cpunode_match(device_t, cfdata_t, void *aux);
static void pq3pci_cpunode_attach(device_t, device_t, void *aux);
static pci_chipset_tag_t pq3pci_pci_chipset_init(struct pq3pci_softc *);

static SIMPLEQ_HEAD(,pq3pci_intrsource) pq3pci_intrsources
    = SIMPLEQ_HEAD_INITIALIZER(pq3pci_intrsources);
static struct pq3pci_msigroup *pq3pci_msigroups[8];

static struct pq3pci_intrsource *
	pq3pci_intr_source_lookup(struct pq3pci_softc *, pci_intr_handle_t);

static const char msi_intr_names[8][32][8] = {
    {
	"msi 0",   "msi 1",   "msi 2",   "msi 3",
	"msi 4",   "msi 5",   "msi 6",   "msi 7",
	"msi 8",   "msi 9",   "msi 10",  "msi 11",
	"msi 12",  "msi 13",  "msi 14",  "msi 15",
	"msi 16",  "msi 17",  "msi 18",  "msi 19",
	"msi 20",  "msi 21",  "msi 22",  "msi 23",
	"msi 24",  "msi 25",  "msi 26",  "msi 27",
	"msi 28",  "msi 29",  "msi 30",  "msi 31",
    }, {
	"msi 32",  "msi 33",  "msi 34",  "msi 35",
	"msi 36",  "msi 37",  "msi 38",  "msi 39",
	"msi 40",  "msi 41",  "msi 42",  "msi 43",
	"msi 44",  "msi 45",  "msi 46",  "msi 47",
	"msi 48",  "msi 49",  "msi 50",  "msi 51",
	"msi 52",  "msi 53",  "msi 54",  "msi 55",
	"msi 56",  "msi 57",  "msi 58",  "msi 59",
	"msi 60",  "msi 61",  "msi 62",  "msi 63",
    }, {
	"msi 64",  "msi 65",  "msi 66",  "msi 67",
	"msi 68",  "msi 69",  "msi 70",  "msi 71",
	"msi 72",  "msi 73",  "msi 74",  "msi 75",
	"msi 76",  "msi 77",  "msi 78",  "msi 79",
	"msi 80",  "msi 81",  "msi 82",  "msi 83",
	"msi 84",  "msi 85",  "msi 86",  "msi 87",
	"msi 88",  "msi 89",  "msi 90",  "msi 91",
	"msi 92",  "msi 93",  "msi 94",  "msi 95",
    }, {
	"msi 96",  "msi 97",  "msi 98",  "msi 99",
	"msi 100", "msi 101", "msi 102", "msi 103",
	"msi 104", "msi 105", "msi 106", "msi 107",
	"msi 108", "msi 109", "msi 110", "msi 111",
	"msi 112", "msi 113", "msi 114", "msi 115",
	"msi 116", "msi 117", "msi 118", "msi 119",
	"msi 120", "msi 121", "msi 122", "msi 123",
	"msi 124", "msi 125", "msi 126", "msi 127",
    }, {
	"msi 128", "msi 129", "msi 130", "msi 131",
	"msi 132", "msi 133", "msi 134", "msi 135",
	"msi 136", "msi 137", "msi 138", "msi 139",
	"msi 140", "msi 141", "msi 142", "msi 143",
	"msi 144", "msi 145", "msi 146", "msi 147",
	"msi 148", "msi 149", "msi 150", "msi 151",
	"msi 152", "msi 153", "msi 154", "msi 155",
	"msi 156", "msi 157", "msi 158", "msi 159",
    }, {
	"msi 160", "msi 161", "msi 162", "msi 163",
	"msi 164", "msi 165", "msi 166", "msi 167",
	"msi 168", "msi 169", "msi 170", "msi 171",
	"msi 172", "msi 173", "msi 174", "msi 175",
	"msi 176", "msi 177", "msi 178", "msi 179",
	"msi 180", "msi 181", "msi 182", "msi 183",
	"msi 184", "msi 185", "msi 186", "msi 187",
	"msi 188", "msi 189", "msi 190", "msi 191",
    }, {
	"msi 192", "msi 193", "msi 194", "msi 195",
	"msi 196", "msi 197", "msi 198", "msi 199",
	"msi 200", "msi 201", "msi 202", "msi 203",
	"msi 204", "msi 205", "msi 206", "msi 207",
	"msi 208", "msi 209", "msi 210", "msi 211",
	"msi 212", "msi 213", "msi 214", "msi 215",
	"msi 216", "msi 217", "msi 218", "msi 219",
	"msi 220", "msi 221", "msi 222", "msi 223",
    }, {
	"msi 224", "msi 225", "msi 226", "msi 227",
	"msi 228", "msi 229", "msi 230", "msi 231",
	"msi 232", "msi 233", "msi 234", "msi 235",
	"msi 236", "msi 237", "msi 238", "msi 239",
	"msi 240", "msi 241", "msi 242", "msi 243",
	"msi 244", "msi 245", "msi 246", "msi 247",
	"msi 248", "msi 249", "msi 250", "msi 251",
	"msi 252", "msi 253", "msi 254", "msi 255",
    },
};

CFATTACH_DECL_NEW(pq3pci_cpunode, sizeof(struct pq3pci_softc),
    pq3pci_cpunode_match, pq3pci_cpunode_attach, NULL, NULL);

CFATTACH_DECL_NEW(pq3pcie_cpunode, sizeof(struct pq3pci_softc),
    pq3pci_cpunode_match, pq3pci_cpunode_attach, NULL, NULL);

int
pq3pci_cpunode_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!e500_cpunode_submatch(parent, cf, cf->cf_name + 3, aux))
		return 0;

	return 1;
}

struct pq3pci_owin {
	uint32_t potar;
	uint32_t potear;
	uint32_t powbar;
	uint32_t powar;
};

static void
pq3pci_owin_record(struct pq3pci_softc *sc, u_int winnum,
	const struct pq3pci_owin *owin)
{
	const bool io_win = (owin->powar & PEXOWAR_RTT) == PEXOWAR_RTT_IO;
	struct pq3pci_bst *bs = io_win ? &sc->sc_pci_io_bst : &sc->sc_pci_mem_bst;
	const uint64_t pci_base = ((uint64_t)owin->potar << 12)
	    | ((uint64_t)owin->potear << (32+12));
	const uint64_t local_base = (uint64_t)owin->powbar << 12;
	const u_int win_size_log2 = PEXIWAR_IWS_GET(owin->powar) + 1;
	u_int slot;

	bs->bs_tag.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN
	    | (io_win ? _BUS_SPACE_IO_TYPE : _BUS_SPACE_MEM_TYPE);

	for (slot = 0; slot < bs->bs_numwin; slot++) {
		if (pci_base < bs->bs_base[slot]) {
			for (size_t j = slot; j < bs->bs_numwin; j++) {
				bs->bs_base[j+1] = bs->bs_base[j];
				bs->bs_offset[j+1] = bs->bs_offset[j];
				bs->bs_limit[j+1] = bs->bs_limit[j];
			}
			break;
		}
	}
	bs->bs_base[slot] = pci_base;
	bs->bs_offset[slot] = local_base - pci_base;
	bs->bs_limit[slot] = pci_base + (1ULL << win_size_log2);
	bs->bs_numwin++;

#if 0
	const char units[] = " KMGTP";
	aprint_normal_dev(sc->sc_dev,
	     "outbound window %u: potar=%#x, potear=%#x, powbar=%x, powar=%#x\n",
	     winnum, owin->potar, owin->potear, owin->powbar, owin->powar);
	aprint_normal_dev(sc->sc_dev,
	    "outbound window %u: maps %u%cB of PCI %s space @ %#"PRIx64" onto local addresses @ %#"PRIx64".\n",
	    winnum, 1 << (win_size_log2 % 10), units[win_size_log2 / 10],
	    (owin->powar & PEXOWAR_RTT) == PEXOWAR_RTT_IO ? "I/O" : "memory",
	    local_base, pci_base);
#endif
}

static bool
pq3pci_owin_init(struct pq3pci_softc *sc, struct pq3pci_bst *bs, bool io_win)
{
	if (bs->bs_numwin == 0)
		return true;

	bs->bs_tag.pbs_base = bs->bs_base[0];
	bs->bs_tag.pbs_offset = bs->bs_offset[0];
	bs->bs_tag.pbs_limit = bs->bs_limit[bs->bs_numwin - 1];

	snprintf(bs->bs_name, sizeof(bs->bs_name), "%s-%s",
	    device_xname(sc->sc_dev), io_win ? "io" : "mem");

#if 0
	printf("%s: %s: base=%#x offset=%#x limit=%#x\n", __func__, bs->bs_name,
	    bs->bs_tag.pbs_base, bs->bs_tag.pbs_offset, bs->bs_tag.pbs_limit);
#endif

	int error = bus_space_init(&bs->bs_tag, bs->bs_name,
	    bs->bs_ex_storage, sizeof(bs->bs_ex_storage));
	if (error) {
		aprint_error(": failed to create %s bus space: %d\n",
		    bs->bs_name, error);
		return false;
	}
	for (size_t slot = 1; slot < bs->bs_numwin; slot++) {
		if (bs->bs_limit[slot - 1] < bs->bs_base[slot]) {
			error = extent_alloc_region(bs->bs_tag.pbs_extent,
			    bs->bs_limit[slot - 1],
			    bs->bs_base[slot] - bs->bs_limit[slot - 1],
			    EX_WAITOK);
			if (error) {
				aprint_error(": failed to hole in %s bus space: %d\n",
				    bs->bs_name, error);
				return false;
			}
		}
	}
	aprint_debug_dev(sc->sc_dev, "bus space %s created\n", bs->bs_name);
	sc->sc_pba_flags |=
	    io_win ? PCI_FLAGS_IO_OKAY : PCI_FLAGS_MEM_OKAY;
	return true;
}

struct pq3pci_iwin {
	uint32_t pitar;
	uint32_t piwbar;
	uint32_t piwbear;
	uint32_t piwar;
};

static bool
pq3pci_iwin_setup(struct pq3pci_softc *sc, u_int winnum,
	const struct pq3pci_iwin *iwin)
{
	const uint64_t pci_base = ((uint64_t)iwin->piwbar << 12)
	    | ((uint64_t)iwin->piwbear << (32+12));
	const uint64_t local_base = (uint64_t)iwin->pitar << 12;
	const u_int win_size_log2 = PEXIWAR_IWS_GET(iwin->piwar) + 1;
#if DEBUG > 1
	const char units[] = " KMGTP";
	aprint_normal_dev(sc->sc_dev,
	    "inbound window %u: pitar=%#x, piwbar=%x, piwbear=%#x, piwar=%#x\n",
	    winnum, iwin->pitar, iwin->piwbar, iwin->piwbear, iwin->piwar);
	aprint_normal_dev(sc->sc_dev,
	    "inbound window %u: maps %u%cB of PCI address space @ %#"PRIx64" to local memory @ %#"PRIx64".\n",
	    winnum, 1 << (win_size_log2 % 10), units[win_size_log2 / 10],
	    pci_base, local_base);
#endif /* DEBUG */
	/*
	 * Let's make sure this window is usable.
	 */
	if (pci_base != 0) {
		aprint_error(": invalid inbound window: "
		    "PCI base (%#"PRIx64" != 0\n", pci_base);
		return false;
	}
	if (local_base != 0) {
		aprint_error(": invalid inbound window: "
		    "local base (%#"PRIx64" != 0\n", local_base);
		return false;
	}
	if ((iwin->piwar & PEXIWAR_RTT) != PEXIWAR_RTT_MEM_SNOOP) {
		aprint_error(": invalid inbound window: "
		    "unsupported read transaction type (%#"PRIxMAX")\n",
		    iwin->piwar & PEXIWAR_RTT);
		return false;
	}
	if ((iwin->piwar & PEXIWAR_WTT) != PEXIWAR_WTT_MEM_SNOOP) {
		aprint_error(": invalid inbound window: "
		    "unsupported write transaction type (%#"PRIxMAX")\n",
		    iwin->piwar & PEXIWAR_WTT);
		return false;
	}
	if ((iwin->piwar & PEXIWAR_TRGT) != PEXIWAR_TRGT_LOCALMEM) {
		aprint_error(": invalid inbound window: "
		    "unsupported target (%#"PRIxMAX")\n",
		    iwin->piwar & PEXIWAR_TRGT);
		return false;
	}
	if (board_info_get_number("mem-size") > (1ULL << win_size_log2)) {
		aprint_error(": invalid inbound window: "
		    "doesn't map all of memory (%#"PRIx64" < %#"PRIx64")\n",
		    1ULL << win_size_log2, board_info_get_number("mem-size"));
		return false;
	}
	return true;
}

static void
pq3pci_pch_callout(void *v)
{
	struct pq3pci_callhand * const pch = v;

	int s = splraise(pch->pch_ipl);
	(*pch->pch_ih.ih_func)(pch->pch_ih.ih_arg);
	splx(s);
	callout_schedule(&pch->pch_callout, 1);
}

static int
pq3pci_msi_spurious_intr(void *v)
{
	(void) v;

	return 0;
}

static int
pq3pci_msi_intr(void *v)
{
	struct pq3pci_msigroup * const msig = v;

	mutex_spin_enter(msig->msig_lock);
	KASSERT(curcpu()->ci_cpl == msig->msig_ipl);
	//KASSERT(curcpu()->ci_idepth == 0);
	uint32_t matches = 0;
	for (int rv = 0;;) {
		uint32_t group = cpu_read_4(msig->msig_msir);
		if (group == 0) {
			mutex_spin_exit(msig->msig_lock);
			return rv;
		}

		const bool working_msi_p =
		    msig->msig_group != 0 || (group & 1) == 0;
		if (working_msi_p) {
			/*
			 * if MSIs are working, just clear the free MSIs.
			 */
			KASSERTMSG((group & msig->msig_free_mask) == 0,
			   "%s: group#%u: unexpected MSIs (%#x)",
			    __func__, msig->msig_group,
			    group & msig->msig_free_mask);
			group &= ~msig->msig_free_mask;
		} else {
			/*
			 * If MSIs are broken, we don't really what MSIs
			 * have happened.
			 */
			for (struct pq3pci_msihand *msih = msig->msig_ihands + 31;
			     group != 0;
			     msih--) {
				const u_int n = __builtin_clz(group);
				msih -= n;
				group <<= n + 1;
				msih->msih_ev.ev_count++;
			}
			group = ~msig->msig_free_mask;
		}
		uint32_t this_msi = __BIT(31);
		for (struct pq3pci_msihand *msih = msig->msig_ihands + 31;
		     group != 0;
		     msih--) {
			KASSERT(msig->msig_ihands <= msih);
			KASSERT(msih < &msig->msig_ihands[32]);
			const u_int n = __builtin_clz(group);
			msih -= n;
			group <<= n + 1;
			msih->msih_ev.ev_count += working_msi_p;
			if ((*msih->msih_ih.ih_func)(msih->msih_ih.ih_arg)) {
				rv = 1;
				msih->msih_ev.ev_count += !working_msi_p;
				matches |= this_msi;
			} else if ((matches & this_msi) == 0) {
				msih->msih_ev_spurious.ev_count += working_msi_p;
			}
			this_msi >>= n + 1;
		}
	}
}

static int
pq3pci_onchip_intr(void *v)
{
	panic(__func__);
}

static int
pq3pci_pis_intr(void *v)
{
	struct pq3pci_intrsource * const pis = v;
	struct pq3pci_intrhand *pih;
	int rv = 0;

	mutex_spin_enter(pis->pis_lock);
	pis->pis_ev.ev_count++;
	SIMPLEQ_FOREACH(pih, &pis->pis_ihands, pih_link) {
		struct pq3pci_softc * const sc = pih->pih_ih.ih_sc;
		int s = splraise(pih->pih_ipl);
		pih->pih_count++;
		rv = (*pih->pih_ih.ih_func)(pih->pih_ih.ih_arg);
		splx(s);
#if 0
		printf("%s %d:%s %"PRIu64": %p(%p) %"PRIu64": %d\n", __func__,
		    curcpu()->ci_idepth,
		    pis->pis_ev.ev_group, pis->pis_ev.ev_count,
		    pih->pih_ih.ih_func, pih->pih_ih.ih_arg, pih->pih_count, rv);
#endif
		if (rv != 0) {
			bus_space_read_4(sc->sc_bst, sc->sc_bsh, PCI_INT_ACK);
			break;
		}
		pih->pih_count--;
	}
	if (rv == 0)
		pis->pis_ev_spurious.ev_count++;
	mutex_spin_exit(pis->pis_lock);
	return rv;
}

static void
pq3pci_intr_source_setup(struct pq3pci_softc *sc,
	struct pq3pci_intrsource *pis, pci_intr_handle_t handle)
{
	char buf[PCI_INTRSTR_LEN];
	SIMPLEQ_INIT(&pis->pis_ihands);
	pis->pis_handle = handle;
	pis->pis_ih = intr_establish(PIH_IRQ(handle), IPL_VM, PIH_IST(handle),
	    pq3pci_pis_intr, pis);
	pis->pis_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);
	const char * const intrstr
	    = intr_string(PIH_IRQ(handle), PIH_IST(handle), buf, sizeof(buf));
	evcnt_attach_dynamic(&pis->pis_ev, EVCNT_TYPE_INTR,
	    NULL, intrstr, "intr");
	evcnt_attach_dynamic(&pis->pis_ev_spurious, EVCNT_TYPE_INTR,
	    &pis->pis_ev, intrstr, "spurious intr");
	SIMPLEQ_INSERT_TAIL(&pq3pci_intrsources, pis, pis_link);
}

static bool
pq3pci_intrmap_setup(struct pq3pci_softc *sc,
	const struct cpunode_locators *cnl)
{
	char prop_name[32];
	snprintf(prop_name, sizeof(prop_name), "%s%u-interrupt-map",
	    cnl->cnl_name, cnl->cnl_instance);
	sc->sc_intrmap = board_info_get_object(prop_name);
	if (sc->sc_intrmap == NULL) {
		aprint_error(": missing %s board property", prop_name);
		return false;
	}

	KASSERT(prop_object_type(sc->sc_intrmap) == PROP_TYPE_DICTIONARY);
	prop_number_t pn = prop_dictionary_get(sc->sc_intrmap, "interrupt-mask");
	KASSERT(pn != NULL);

	sc->sc_intrmask = prop_number_unsigned_integer_value(pn);

	sc->sc_ih = intr_establish(cnl->cnl_intrs[0], IPL_VM, IST_ONCHIP,
	    pq3pci_onchip_intr, sc);
	if (sc->sc_ih == NULL)
		panic("%s: failed to establish interrupt %d\n",
		    device_xname(sc->sc_dev), cnl->cnl_intrs[0]);

	return true;
}

void
pq3pci_cpunode_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct pq3pci_softc * const sc = device_private(self);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;
	char buf[32];

	sc->sc_dev = self;
	sc->sc_bst = cna->cna_memt;
	psc->sc_children |= cna->cna_childmask;
	sc->sc_pcie = strcmp(cnl->cnl_name, "pcie") == 0;

	const uint32_t pordevsr = cpu_read_4(GLOBAL_BASE + PORDEVSR);
	if (sc->sc_pcie) {
		u_int lanes = e500_truth_decode(cnl->cnl_instance, pordevsr,
		    pq3pci_pcie_lanes, __arraycount(pq3pci_pcie_lanes), 0);
		if (lanes == 0) {
			aprint_normal(": disabled\n");
			return;
		}
		snprintf(buf, sizeof(buf), "PCI-Express x%u", lanes);
	} else {
		bool pcix_p = e500_truth_decode(cnl->cnl_instance, pordevsr,
		    pq3pci_pci_pcix, __arraycount(pq3pci_pci_pcix), 0);
		u_int width = e500_truth_decode(cnl->cnl_instance, pordevsr,
		    pq3pci_pci_pci32, __arraycount(pq3pci_pci_pci32), 32);
		snprintf(buf, sizeof(buf), "%u-bit PCI%s",
		    width, (pcix_p ? "X" : ""));
	}

	if (!pq3pci_intrmap_setup(sc, cnl))
		return;

	evcnt_attach_dynamic(&sc->sc_ev_spurious, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "spurious intr");

	int error = bus_space_map(sc->sc_bst, cnl->cnl_addr, cnl->cnl_size, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error(": failed to map registers: %d\n", error);
		return;
	}

	u_int valid_owins = 0;
	for (u_int i = 1, off = PEXOTAR1 - PEXOTAR0;
	     i < 4; i++, off += PEXOTAR1 - PEXOTAR0) {
		struct pq3pci_owin owin;
		owin.potar = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    PEXOTAR0 + off);
		owin.potear = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    PEXOTEAR0 + off);
		owin.powbar = 0;
		if (i > 0) {
			/* Doesn't exist for outbound window 0 */
			owin.powbar = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
			    PEXOWBAR1 - (PEXOTAR1 - PEXOTAR0) + off);
		}
		owin.powar = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    PEXOWAR0 + off);
#if 0
		aprint_normal_dev(self,
		    "owin[%u]: potar=%#x potear=%#x powbar=%#x powar=%#x\n",
		    i, owin.potar, owin.potear, owin.powbar, owin.powar);
#endif
		if (owin.powar & PEXOWAR_EN) {
			valid_owins++;
			pq3pci_owin_record(sc, i, &owin);
		}
	}
	if (!pq3pci_owin_init(sc, &sc->sc_pci_io_bst, true)
	    || !pq3pci_owin_init(sc, &sc->sc_pci_mem_bst, false)) {
		return;
	}
#ifndef PCI_NETBSD_CONFIGURE
	if (valid_owins == 0) {
		aprint_normal(": %s controller%s\n", buf,
		    " (disabled)");
		return;
	}
#endif

	u_int valid_iwins = 0;
	for (u_int i = 0, off = 0; i < 3; i++, off += PEXITAR2 - PEXITAR1) {
		struct pq3pci_iwin iwin;
		iwin.pitar = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    PEXITAR1 + off);
		iwin.piwbar = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    PEXIWBAR1 + off);
		if (i > 0) {
			/* Doesn't exist */
			iwin.piwbear = bus_space_read_4(sc->sc_bst,
			    sc->sc_bsh,
			    PEXIWBEAR2 - (PEXITAR2 - PEXITAR1) + off);
		} else {
			iwin.piwbear = 0;
		}
		iwin.piwar = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    PEXIWAR1 + off);
#if 0
		aprint_normal_dev(self,
		    "iwin[%u]: pitar=%#x piwbar=%#x piwbear=%#x piwar=%#x\n",
		    i, iwin.pitar, iwin.piwbar, iwin.piwbear, iwin.piwar);
#endif
		if (iwin.piwar & PEXIWAR_EN) {
			valid_iwins++;
			if (!pq3pci_iwin_setup(sc, i, &iwin))
				return;
		}
	}

	sc->sc_conf_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);

	pci_chipset_tag_t pc = pq3pci_pci_chipset_init(sc);

#ifndef PCI_NETBSD_CONFIGURE
	if (valid_iwins == 0) {
		aprint_normal(": %s controller%s\n", buf,
		    " (disabled)");
		return;
	}
#else
	if (sc->sc_pcie && pci_conf_read(pc, 0, PEX_LTSSM) < LTSSM_L0) {
		aprint_normal(": %s controller%s\n", buf,
		    " (offline)");
		return;
	}
	if (!sc->sc_pcie && (pci_conf_read(pc, 0, PCI_PBFR) & PBFR_PAH)) {
		aprint_normal(": %s controller%s\n", buf,
		    " (agent mode)");
		return;
	}
	if (valid_iwins == 0) {
		struct pq3pci_iwin iwin = {
		    .pitar = 0,
		    .piwbar = 0,
		    .piwbear = 0,
		    .piwar = PEXIWAR_EN|PEXIWAR_PF|PEXIWAR_TRGT_LOCALMEM
			|PEXIWAR_RTT_MEM_SNOOP|PEXIWAR_WTT_MEM_SNOOP
			|__SHIFTIN(30-__builtin_clz(pmemsize),PEXIWAR_IWS),
		};
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXITAR2, iwin.pitar);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXIWBAR2, iwin.piwbar);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXIWBEAR2, iwin.piwbear);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXIWAR2, iwin.piwar);

		if (!pq3pci_iwin_setup(sc, 2, &iwin)) {
			aprint_error(": error creating inbound window\n");
			return;
		}

	}

	if (valid_owins == 0) {
		u_long membase, iobase;
		error = extent_alloc(pcimem_ex, PCI_MEMSIZE, PCI_MEMSIZE,
		   PCI_MEMSIZE, EX_WAITOK, &membase);
		if (error) {
			aprint_error(
			    ": error allocating address space for %s: %d\n",
			    "PCI memory", error);
			return;
		}
		struct pq3pci_owin owin1 = {
		    .potar = membase >> 12,
		    .potear = 0,
		    .powbar = membase >> 12,
		    .powar = PEXOWAR_EN|PEXOWAR_TC0
			|PEXOWAR_RTT_MEM|PEXOWAR_WTT_MEM
			|__SHIFTIN(ilog2(PCI_MEMSIZE)-1,PEXOWAR_OWS),
		};
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXOTAR1, owin1.potar);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXOTEAR1, owin1.potear);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXOWBAR1, owin1.powbar);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXOWAR1, owin1.powar);
		pq3pci_owin_record(sc, 1, &owin1);
		if (!pq3pci_owin_init(sc, &sc->sc_pci_mem_bst, false)) {
			return;
		}

		error = extent_alloc(pciio_ex, PCI_IOSIZE, PCI_IOSIZE,
		   PCI_IOSIZE, EX_WAITOK, &iobase);
		if (error) {
			aprint_error(
			    ": error allocating address space for %s: %d\n",
			    "PCI I/O space", error);
			return;
		}
		struct pq3pci_owin owin2 = {
		    .potar = 0,
		    .potear = 0,
		    .powbar = iobase >> 12,
		    .powar = PEXOWAR_EN|PEXOWAR_TC0
			|PEXOWAR_RTT_IO|PEXOWAR_WTT_IO
			|__SHIFTIN(ilog2(PCI_IOSIZE)-1,PEXOWAR_OWS),
		};
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXOTAR2, owin2.potar);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXOTEAR2, owin2.potear);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXOWBAR2, owin2.powbar);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEXOWAR2, owin2.powar);
		pq3pci_owin_record(sc, 2, &owin1);
		if (!pq3pci_owin_init(sc, &sc->sc_pci_io_bst, true)) {
			return;
		}

		struct extent *ioext = extent_create("pciio", 0, PCI_IOSIZE,
		     NULL, 0, EX_NOWAIT);
		struct extent *memext = extent_create("pcimem", membase,
		     membase + PCI_MEMSIZE, NULL, 0, EX_NOWAIT);

		error = pci_configure_bus(pc, ioext, memext, NULL, 0,
		    curcpu()->ci_ci.dcache_line_size);

		extent_destroy(ioext);
		extent_destroy(memext);

		if (error) {
			aprint_normal(": configuration failed\n");
			return;
		}
	}
#endif

	aprint_normal(": %s controller%s\n", buf, "");

	struct pcibus_attach_args pba;
	memset(&pba, 0, sizeof(pba));

	pba.pba_flags = sc->sc_pba_flags | PCI_FLAGS_MSI_OKAY
	    | PCI_FLAGS_MSIX_OKAY;
	if (pba.pba_flags & PCI_FLAGS_IO_OKAY)
		pba.pba_iot = pc->pc_iot;
	if (pba.pba_flags & PCI_FLAGS_MEM_OKAY)
		pba.pba_memt = pc->pc_memt;
	pba.pba_dmat = cna->cna_dmat;
	pba.pba_pc = pc;
	pba.pba_bus = 0;

	/*
	 * Program BAR0 so that MSIs can work.
	 */
	pci_conf_write(pc, 0, PCI_BAR0, sc->sc_bst->pbs_offset);
	pcireg_t cmdsts = pci_conf_read(pc, 0, PCI_COMMAND_STATUS_REG);
	cmdsts |= PCI_COMMAND_INTERRUPT_DISABLE;
	pci_conf_write(pc, 0, PCI_COMMAND_STATUS_REG, cmdsts);

#if 0
	/*
	 *
	 */
	pq3pci_intr_source_lookup(sc, PIH_MAKE(0, IST_LEVEL, 0));
#endif
#if 0
	if (sc->sc_pcie)
		pci_conf_print(pc, 0, NULL);
#endif

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static void
pq3pci_attach_hook(device_t parent, device_t self,
	struct pcibus_attach_args *pba)
{
	/* do nothing */
}

static int
pq3pci_bus_maxdevs(void *v, int busno)
{
	struct pq3pci_softc * const sc = v;
	return sc->sc_pcie && busno < 2 ? 1 : 32;
}

static void
pq3pci_decompose_tag(void *v, pcitag_t tag, int *bus, int *dev, int *func)
{
	if (bus)
		*bus = (tag >> 16) & 0xff;
	if (dev)
		*dev = (tag >> 11) & 0x1f;
	if (func)
		*func = (tag >> 8) & 0x07;
}

static pcitag_t
pq3pci_make_tag(void *v, int bus, int dev, int func)
{
	return (bus << 16) | (dev << 11) | (func << 8);
}

#if 0
static inline pcitag_t
pq3pci_config_addr_read(pci_chipset_tag_t pc)
{
	pcitag_t v;
        __asm volatile("lwz\t%0, 0(%1)" : "=r"(v) : "b"(pc->pc_addr));
        __asm volatile("mbar\n\tmsync");
	return v;
}
#endif

static inline void
pq3pci_config_addr_write(pci_chipset_tag_t pc, pcitag_t v)
{
        __asm volatile("stw\t%0, 0(%1)" :: "r"(v), "b"(pc->pc_addr));
        __asm volatile("mbar\n\tmsync");
}

static inline pcireg_t
pq3pci_config_data_read(pci_chipset_tag_t pc)
{
	pcireg_t v;
        __asm volatile("lwbrx\t%0, 0, %1" : "=r"(v) : "b"(pc->pc_data));
        __asm volatile("mbar\n\tmsync");
	return v;
}

static inline void
pq3pci_config_data_write(pci_chipset_tag_t pc, pcireg_t v)
{
        __asm volatile("stwbrx\t%0, 0, %1" :: "r"(v), "r"(pc->pc_data));
        __asm volatile("mbar\n\tmsync");
}

static pcireg_t
pq3pci_conf_read(void *v, pcitag_t tag, int reg)
{
	struct pq3pci_softc * const sc = v;
	struct genppc_pci_chipset * const pc = &sc->sc_pc;

	if (reg >= 256) {
		if (!sc->sc_pcie)
			return 0xffffffff;
		reg = (reg & 0xff) | ((reg & 0xf00) << 16);
	}
	if (sc->sc_pcie && ((tag >> 16) & 0xff) != 0) {
	//	pcireg_t slot_status = pci_conf_read(pc, 0, 0x64);
	//	printf("%s: tag 0x0 slot status: %#x\n",__func__, slot_status);
	//	if ((slot_status & __BIT(6+16)) == 0)
	//		printf(" addr=%#llx ", tag | reg | PEX_CONFIG_ADDR_EN);
	//		return 0xffffffff;
	}

	mutex_spin_enter(sc->sc_conf_lock);

	pq3pci_config_addr_write(pc, tag | reg | PEX_CONFIG_ADDR_EN);
	pcireg_t rv = pq3pci_config_data_read(pc);

	mutex_spin_exit(sc->sc_conf_lock);

#if 0
	uint32_t err = bus_space_read_4(sc->sc_bst, sc->sc_bsh, PEX_ERR_DR);
	if (err & PEXERRDR_ICCA) {
		aprint_error_dev(sc->sc_dev, "%s: tag %#x reg %#x icca: %#x\n",
		    __func__, tag, reg, pq3pci_config_addr_read(pc));
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PEX_ERR_DR,
		    PEXERRDR_ICCA);
	}
#endif
	return rv;
}

static void
pq3pci_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	struct pq3pci_softc * const sc = v;
	struct genppc_pci_chipset * const pc = &sc->sc_pc;

	if (reg >= 256) {
		if (!sc->sc_pcie)
			return;
		reg = (reg & 0xff) | ((reg & 0xf00) << 16);
	}

	mutex_spin_enter(sc->sc_conf_lock);

#if 0
	aprint_error_dev(sc->sc_dev, "%s: tag %#x reg %#x data %#x\n",
	    __func__, tag, reg, data);
#endif
	pq3pci_config_addr_write(pc, tag | reg | PEX_CONFIG_ADDR_EN);
	pq3pci_config_data_write(pc, data);

	mutex_spin_exit(sc->sc_conf_lock);
}

static int
pq3pci_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{
	struct pq3pci_softc * const sc = v;
	if (sc->sc_pcie && bus != 0) {
		pcireg_t slot_status = pci_conf_read(&sc->sc_pc, 0, 0x64);
		if ((slot_status & __BIT(6+16)) == 0)
			return 0;
	}
	if (!sc->sc_pcie && bus == 0 && dev == 0) {
		return PCI_CONF_DEFAULT ^ (PCI_CONF_MAP_IO|PCI_CONF_MAP_MEM|PCI_CONF_MAP_ROM);
	}
	return PCI_CONF_DEFAULT;
}

static void
pq3pci_msi_group_setup(struct pq3pci_msigroup *msig, u_int group, int ipl)
{
	const char (*intr_names)[8] = msi_intr_names[group];

	KASSERT(ipl == IPL_VM);

	pq3pci_msigroups[group] = msig;
	msig->msig_group = group;
	msig->msig_free_mask = ~0 << (group == 0);
	msig->msig_ipl = ipl;
	msig->msig_lock = mutex_obj_alloc(MUTEX_DEFAULT, ipl);
	msig->msig_ih = intr_establish(msig->msig_group, ipl, IST_MSIGROUP,
	    pq3pci_msi_intr, msig);
	msig->msig_msir = OPENPIC_BASE + OPENPIC_MSIR(msig->msig_group);
	for (u_int i = 0; i < __arraycount(msig->msig_ihands); i++) {
		struct pq3pci_msihand * const msih = msig->msig_ihands + i;
		msih->msih_ih.ih_class = IH_MSI;
		msih->msih_ih.ih_func = pq3pci_msi_spurious_intr;
		msih->msih_ih.ih_arg = msih;
		msih->msih_group = msig;
		evcnt_attach_dynamic(&msih->msih_ev, EVCNT_TYPE_INTR,
		    NULL, intr_names[i], "intr");
		evcnt_attach_dynamic(&msih->msih_ev_spurious, EVCNT_TYPE_INTR,
		    &msih->msih_ev, intr_names[i], "spurious intr");
	}
}

static pci_intr_handle_t
pq3pci_msi_alloc(int ipl, u_int rmsi)
{
	size_t freegroup = 0;
	size_t maplen = __arraycount(pq3pci_msigroups);
	KASSERT(rmsi <= 5);
	uint32_t bitmap[maplen];

	for (u_int i = 0; i < maplen; i++) {
		struct pq3pci_msigroup * const msig = pq3pci_msigroups[i];
		if (msig == NULL) {
			bitmap[i] = 0;
			if (freegroup == 0)
				freegroup = i + 1;
			continue;
		}
		/*
		 * If this msigroup has the wrong IPL or there's nothing
		 * free, try the next one.
		 */
		if (msig->msig_ipl != ipl || msig->msig_free_mask == 0) {
			bitmap[i] = 0;
			continue;
		}

		bitmap[i] = msig->msig_free_mask;
	}
	for (u_int i = 0; i < maplen; i++) {
		uint32_t mapbits = bitmap[i];
		u_int n = ffs(mapbits);
		if (n--) {
			return PIH_MAKE(i * 32 + n, IST_MSI, 0);
		}
	}

	if (freegroup-- == 0)
		return 0;

	struct pq3pci_msigroup * const msig =
	    kmem_zalloc(sizeof(*msig), KM_SLEEP);
	KASSERT(msig != NULL);
	pq3pci_msi_group_setup(msig, freegroup, ipl);
	u_int n = ffs(msig->msig_free_mask) - 1;
	return PIH_MAKE(freegroup * 32 + n, IST_MSI, 0);
}

static struct pq3pci_msihand *
pq3pci_msi_lookup(pci_intr_handle_t handle)
{
	const int irq = PIH_IRQ(handle);
	KASSERT(irq < 256);
	struct pq3pci_msigroup * const msig = pq3pci_msigroups[irq / 32];
	KASSERT(msig != NULL);
	return &msig->msig_ihands[irq & 31];
}

static struct pq3pci_msihand *
pq3pci_msi_claim(pci_intr_handle_t handle)
{
	const int irq = PIH_IRQ(handle);
	uint32_t irq_mask = __BIT(irq & 31);
	KASSERT(irq < 256);
	struct pq3pci_msigroup * const msig = pq3pci_msigroups[irq / 32];
	KASSERT(msig != NULL);
	struct pq3pci_msihand * const msih = &msig->msig_ihands[irq & 31];
	mutex_spin_enter(msig->msig_lock);
	KASSERT(msig->msig_free_mask & irq_mask);
	msig->msig_free_mask ^= irq_mask;
	mutex_spin_exit(msig->msig_lock);
	return msih;
}

static struct pq3pci_intrsource *
pq3pci_intr_source_lookup(struct pq3pci_softc *sc, pci_intr_handle_t handle)
{
	struct pq3pci_intrsource *pis;
	SIMPLEQ_FOREACH(pis, &pq3pci_intrsources, pis_link) {
		if (pis->pis_handle == handle)
			return pis;
	}
	pis = kmem_zalloc(sizeof(*pis), KM_SLEEP);
	pq3pci_intr_source_setup(sc, pis, handle);
	return pis;
}

static pci_intr_handle_t
pq3pci_intr_handle_lookup(struct pq3pci_softc *sc,
    const struct pci_attach_args *pa)
{
	prop_dictionary_t entry;

	if (sc->sc_pcie) do {
		pcireg_t msictl;
		int msioff;
		if (!pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_MSI,
					&msioff, &msictl))
			break;
		msictl = pci_conf_read(pa->pa_pc, pa->pa_tag, msioff);
		msictl &= ~PCI_MSI_CTL_MSI_ENABLE;
		msictl &= ~PCI_MSI_CTL_MME_MASK;
		int rmsi = __SHIFTOUT(msictl, PCI_MSI_CTL_MMC_MASK);
		pci_conf_write(pa->pa_pc, pa->pa_tag, msioff, msictl);
		pci_intr_handle_t handle = pq3pci_msi_alloc(IPL_VM, rmsi);
		struct pq3pci_msihand * const msih = pq3pci_msi_lookup(handle);
		msih->msih_tag = pa->pa_tag;
		msih->msih_msioff = msioff;
		return handle;
	} while (false);


	if (sc->sc_intrmask == 0) {
		entry = prop_dictionary_get(sc->sc_intrmap, "000000");
	} else {
		char prop_name[8];
		u_int intrinc = __LOWEST_SET_BIT(sc->sc_intrmask);
		pcitag_t tag = (pa->pa_intrpin - PCI_INTERRUPT_PIN_A) * intrinc;

		snprintf(prop_name, sizeof(prop_name), "%06x",
		    tag & sc->sc_intrmask);

#if 0
		printf("%s: %#x %#x %u (%u) -> %#x & %#x -> %#x <%s>\n",
		    __func__, pa->pa_tag, pa->pa_intrtag, pa->pa_intrpin, pa->pa_rawintrpin,
		    tag, sc->sc_intrmask, tag & sc->sc_intrmask, prop_name);
#endif

		entry = prop_dictionary_get(sc->sc_intrmap, prop_name);
	}
	KASSERT(entry != NULL);
	KASSERT(prop_object_type(entry) == PROP_TYPE_DICTIONARY);

	prop_number_t pn_irq = prop_dictionary_get(entry, "interrupt");
	KASSERT(pn_irq != NULL);
	KASSERT(prop_object_type(pn_irq) == PROP_TYPE_NUMBER);
	int irq = prop_number_unsigned_integer_value(pn_irq);
	prop_number_t pn_ist = prop_dictionary_get(entry, "type");
	KASSERT(pn_ist != NULL);
	KASSERT(prop_object_type(pn_ist) == PROP_TYPE_NUMBER);
	int ist = prop_number_unsigned_integer_value(pn_ist);

	return PIH_MAKE(irq, ist, 0);
}

static int
pq3pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *handlep)
{
	struct pq3pci_softc * const sc = pa->pa_pc->pc_intr_v;

	if (pa->pa_intrpin == PCI_INTERRUPT_PIN_NONE)
		return ENOENT;

	*handlep = pq3pci_intr_handle_lookup(sc, pa);

	return 0;
}

static const char *
pq3pci_intr_string(void *v, pci_intr_handle_t handle, char *buf, size_t len)
{
	if (PIH_IST(handle) == IST_MSI) {
		const char (*intr_names)[8] = msi_intr_names[0];
		strlcpy(buf, intr_names[PIH_IRQ(handle)], len);
		return buf;
	}

	return intr_string(PIH_IRQ(handle), PIH_IST(handle), buf, len);
}

static const struct evcnt *
pq3pci_intr_evcnt(void *v, pci_intr_handle_t handle)
{
	struct pq3pci_softc * const sc = v;
	struct pq3pci_intrsource * const pis =
	    pq3pci_intr_source_lookup(sc, handle);

	KASSERT(pis != NULL);

	return &pis->pis_ev;
}

static void *
pq3pci_intr_establish(void *v, pci_intr_handle_t handle, int ipl,
	int (*func)(void *), void *arg)
{
	struct pq3pci_softc * const sc = v;

	if (0) {
		struct pq3pci_callhand * const pch =
		    kmem_zalloc(sizeof(*pch), KM_SLEEP);
		KASSERT(pch);
		pch->pch_ih.ih_arg = arg;
		pch->pch_ih.ih_func = func;
		pch->pch_ih.ih_sc = sc;
		pch->pch_ipl = ipl;

		callout_init(&pch->pch_callout, 0);
		callout_reset(&pch->pch_callout, 1, pq3pci_pch_callout, pch);

		return pch;
	}

	const int ist = PIH_IST(handle);

	if (ist == IST_MSI) {
		pci_chipset_tag_t pc = &sc->sc_pc;
		struct pq3pci_msihand * const msih = pq3pci_msi_claim(handle);
		pcireg_t cmdsts, msictl;

		if (msih == NULL)
			return NULL;

		struct pq3pci_msigroup * const msig = msih->msih_group;
		const pcitag_t tag = msih->msih_tag;

		mutex_spin_enter(msig->msig_lock);
		msih->msih_ih.ih_class = IH_MSI;
		msih->msih_ih.ih_arg = arg;
		msih->msih_ih.ih_func = func;
		msih->msih_ih.ih_sc = sc;

		int off = msih->msih_msioff;
		msictl = pci_conf_read(pc, tag, off);

		/*
		 * The PCSRBAR has already been setup as a 1:1 BAR so we point
		 * MSIs at the MSII register in the OpenPIC.
		 */
		off += 4;
		pci_conf_write(pc, tag, off,
		    sc->sc_bst->pbs_offset + OPENPIC_BASE + OPENPIC_MSIIR);	

		/*
		 * Upper address is going to be 0.
		 */
		if (msictl & PCI_MSI_CTL_64BIT_ADDR) {
			off += 4;
			pci_conf_write(pc, tag, off, 0);
		}

		/*
		 * Set the magic value.  Since PCI writes this to the least
		 * significant byte of AD[31:0], let's hope the bridge byte
		 * swaps to so it's the most significant bytes or nothing is
		 * going to happen.
		 */
		off += 4;
		pci_conf_write(pc, tag, off, PIH_IRQ(handle));

		/*
		 * Should the driver do this?  How would it know to do it?
		 */
		if (msictl & PCI_MSI_CTL_PERVEC_MASK) {
			off += 4;
			pci_conf_write(pc, tag, off, 0);
		}
		
		/*
		 * Let's make sure he won't raise any INTx.  Technically
		 * setting MSI enable will prevent that as well but might
		 * as well be as safe as possible.
		 */
		cmdsts = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		cmdsts |= PCI_COMMAND_INTERRUPT_DISABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, cmdsts);

#if 1
		/*
		 * Now we can enable the MSI
		 */
		msictl |= PCI_MSI_CTL_MSI_ENABLE;
		pci_conf_write(pc, tag, msih->msih_msioff, msictl);
#endif

		mutex_spin_exit(msig->msig_lock);

#if 0
		struct pq3pci_callhand * const pch =
		    kmem_zalloc(sizeof(*pch), KM_SLEEP);
		KASSERT(pch);

		pch->pch_ih.ih_arg = msig;
		pch->pch_ih.ih_func = pq3pci_msi_intr;
#if 1
		pch->pch_ih.ih_arg = arg;
		pch->pch_ih.ih_func = func;
#endif
		pch->pch_ih.ih_sc = sc;
		pch->pch_ipl = ipl;

		callout_init(&pch->pch_callout, 0);
		callout_reset(&pch->pch_callout, 1, pq3pci_pch_callout, pch);

#if 1
		return pch;
#endif
#endif

		return msih;
	} else {
		struct pq3pci_intrsource * const pis =
		    pq3pci_intr_source_lookup(sc, handle);
		KASSERT(pis != NULL);

		struct pq3pci_intrhand * const pih =
		    kmem_zalloc(sizeof(*pih), KM_SLEEP);

		if (pih == NULL)
			return NULL;

		pih->pih_ih.ih_class = IH_INTX;
		pih->pih_ih.ih_func = func;
		pih->pih_ih.ih_arg = arg;
		pih->pih_ih.ih_sc = sc;
		pih->pih_ipl = ipl;
		pih->pih_source = pis;

		mutex_spin_enter(pis->pis_lock);
		SIMPLEQ_INSERT_TAIL(&pis->pis_ihands, pih, pih_link);
		mutex_spin_exit(pis->pis_lock);

		return pih;
	}
}

static void
pq3pci_intr_disestablish(void *v, void *ih)
{
	struct pq3pci_genihand * const gih = ih;

	if (gih->ih_class == IH_INTX) {
		struct pq3pci_intrhand * const pih = ih;
		struct pq3pci_intrsource * const pis = pih->pih_source;

		mutex_spin_enter(pis->pis_lock);
		SIMPLEQ_REMOVE(&pis->pis_ihands, pih, pq3pci_intrhand, pih_link);
		mutex_spin_exit(pis->pis_lock);

		kmem_free(pih, sizeof(*pih));
		return;
	}
	struct pq3pci_msihand * const msih = ih;
	struct pq3pci_msigroup * const msig = msih->msih_group;
	struct genppc_pci_chipset * const pc = &msih->msih_ih.ih_sc->sc_pc;
	const pcitag_t tag = msih->msih_tag;

	mutex_spin_enter(msig->msig_lock);

	/*
	 * disable the MSI
	 */
	pcireg_t msictl = pci_conf_read(pc, tag, msih->msih_msioff);
	msictl &= ~PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, msih->msih_msioff, msictl);

	msih->msih_ih.ih_func = pq3pci_msi_spurious_intr;
	msih->msih_ih.ih_arg = msig;
	msih->msih_ih.ih_sc = NULL;
	msih->msih_tag = 0;
	msih->msih_msioff = 0;
	mutex_spin_exit(msig->msig_lock);
}

static void
pq3pci_conf_interrupt(void *v, int bus, int dev, int pin, int swiz, int *iline)
{
}

static pci_chipset_tag_t
pq3pci_pci_chipset_init(struct pq3pci_softc *sc)
{
	struct genppc_pci_chipset * const pc = &sc->sc_pc;

	pc->pc_conf_v = sc;
	pc->pc_attach_hook = pq3pci_attach_hook;
        pc->pc_bus_maxdevs = pq3pci_bus_maxdevs;
        pc->pc_make_tag = pq3pci_make_tag;
        pc->pc_conf_read = pq3pci_conf_read;
        pc->pc_conf_write = pq3pci_conf_write;
#ifdef PCI_NETBSD_CONFIGURE
        pc->pc_conf_hook = pq3pci_conf_hook;
#endif

        pc->pc_intr_v = sc;
        pc->pc_intr_map = pq3pci_intr_map;
        pc->pc_intr_string = pq3pci_intr_string;
        pc->pc_intr_evcnt = pq3pci_intr_evcnt;
        pc->pc_intr_establish = pq3pci_intr_establish;
        pc->pc_intr_disestablish = pq3pci_intr_disestablish;
        pc->pc_conf_interrupt = pq3pci_conf_interrupt;

	pc->pc_msi_v = sc;
	genppc_pci_chipset_msi_init(pc);
#if 0
	pc->pc_msi_request = pq3pci_msi_request;
	pc->pc_msi_available = pq3pci_msi_available;
	pc->pc_msi_type = pq3pci_msi_type;
	pc->pc_msi_string = pq3pci_msi_string;
	pc->pc_msi_evcnt = genppc_pci_msi_evcnt;
	pc->pc_msi_establish = pq3pci_msi_establish;
	pc->pc_msix_establish = pq3pci_msix_establish;
	pc->pc_msi_disestablish = pq3pci_msi_disestablish;
	pc->pc_msi_release = pq3pci_msi_release;
	pc->pc_msi_free = pq3pci_msi_free;
#endif

        pc->pc_decompose_tag = pq3pci_decompose_tag;
        pc->pc_conf_hook = pq3pci_conf_hook;

	/*
	 * This is a horrible kludge but it makes life easier.
	 */
        pc->pc_addr = (void *)(sc->sc_bsh + PEX_CONFIG_ADDR);
        pc->pc_data = (void *)(sc->sc_bsh + PEX_CONFIG_DATA);
        pc->pc_bus = 0;
        pc->pc_memt = &sc->sc_pci_mem_bst.bs_tag;
        pc->pc_iot = &sc->sc_pci_io_bst.bs_tag;

	SIMPLEQ_INIT(&pc->pc_pbi);

	return pc;
}
