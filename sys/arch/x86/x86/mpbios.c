/*	$NetBSD: mpbios.c,v 1.66.8.1 2018/07/28 04:37:42 pgoyette Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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

/*
 * Copyright (c) 1999 Stefan Grefen
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Derived from FreeBSD's mp_machdep.c
 */
/*
 * Copyright (c) 1996, by Steve Passe
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * The Intel MP-stuff is just one way of x86 SMP systems
 * so only Intel MP specific stuff is here.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpbios.c,v 1.66.8.1 2018/07/28 04:37:42 pgoyette Exp $");

#include "acpica.h"
#include "lapic.h"
#include "ioapic.h"
#include "opt_acpi.h"
#include "opt_mpbios.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <machine/specialreg.h>
#include <machine/cpuvar.h>
#include <machine/mpbiosvar.h>
#include <machine/pio.h>

#include <machine/i82093reg.h>
#include <machine/i82093var.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

#include <dev/isa/isareg.h>

#ifdef X86_MPBIOS_SUPPORT_EISA
#include <dev/eisa/eisavar.h>	/* for ELCR* def'ns */
#endif

#if NACPICA > 0
extern int mpacpi_ncpu;
extern int mpacpi_nioapic;
#endif

int mpbios_ncpu;
int mpbios_nioapic;

#include "pci.h"

#if NPCI > 0
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#endif

#include "locators.h"

/* descriptions of MP basetable entries */
struct mpbios_baseentry {
	uint8_t  	type;
	uint8_t  	length;
	uint16_t	count;
	const char    	*name;
};

static const char *loc_where[] = {
	"extended bios data area",
	"last page of base memory",
	"bios"
};

struct mp_map
{
	vaddr_t 	baseva;
	int	 	vsize;
	paddr_t 	pa;
	paddr_t 	pg;
	int		psize;
};

struct dflt_conf_entry {
	const char	*bus_type[2];
	int		flags;
};

int mp_cpuprint(void *, const char *);
int mp_ioapicprint(void *, const char *);
static const void *mpbios_search(device_t, paddr_t, int,
    struct mp_map *);
static inline int mpbios_cksum(const void *,int);

static void mp_cfg_special_intr(const struct mpbios_int *, uint32_t *);
static void mp_print_special_intr (int intr);

static void mp_cfg_pci_intr(const struct mpbios_int *, uint32_t *);
static void mp_print_pci_intr (int intr);

#ifdef X86_MPBIOS_SUPPORT_EISA
static void mp_print_eisa_intr (int intr);
static void mp_cfg_eisa_intr(const struct mpbios_int *, uint32_t *);
#endif

static void mp_cfg_isa_intr(const struct mpbios_int *, uint32_t *);
static void mp_print_isa_intr(int intr);

static void mpbios_dflt_conf_cpu(device_t);
static void mpbios_dflt_conf_bus(device_t, const struct dflt_conf_entry *);
static void mpbios_dflt_conf_ioapic(device_t);
static void mpbios_dflt_conf_int(device_t, const struct dflt_conf_entry *,
                                 const int *);

static void mpbios_cpu(const uint8_t *, device_t);
static void mpbios_bus(const uint8_t *, device_t);
static void mpbios_ioapic(const uint8_t *, device_t);
static void mpbios_int(const uint8_t *, int, struct mp_intr_map *);

static const void *mpbios_map(paddr_t, int, struct mp_map *);
static void mpbios_unmap(struct mp_map *);

/*
 * globals to help us bounce our way through parsing the config table.
 */

static struct mp_map mp_cfg_table_map;
static struct mp_map mp_fp_map;
const struct mpbios_cth	*mp_cth;
const struct mpbios_fps	*mp_fps;

int mpbios_scanned;

int
mp_cpuprint(void *aux, const char *pnp)
{
	struct cpu_attach_args *caa = aux;

	if (pnp)
		aprint_normal("cpu at %s", pnp);
	aprint_normal(" apid %d", caa->cpu_number);
	return (UNCONF);
}

int
mp_ioapicprint(void *aux, const char *pnp)
{
	struct apic_attach_args *aaa = aux;

	if (pnp)
		aprint_normal("ioapic at %s", pnp);
	aprint_normal(" apid %d", aaa->apic_id);
	return (UNCONF);
}

/*
 * Map a chunk of memory read-only and return an appropriately
 * const'ed pointer.
 */

static const void *
mpbios_map(paddr_t pa, int len, struct mp_map *handle)
{
	paddr_t pgpa = x86_trunc_page(pa);
	paddr_t endpa = x86_round_page(pa + len);
	vaddr_t va = uvm_km_alloc(kernel_map, endpa - pgpa, 0, UVM_KMF_VAONLY);
	vaddr_t retva = va + (pa & PGOFSET);

	handle->pa = pa;
	handle->pg = pgpa;
	handle->psize = len;
	handle->baseva = va;
	handle->vsize = endpa-pgpa;

	do {
		pmap_kenter_pa(va, pgpa, VM_PROT_READ, 0);
		va += PAGE_SIZE;
		pgpa += PAGE_SIZE;
	} while (pgpa < endpa);
	pmap_update(pmap_kernel());

	return (const void *)retva;
}

inline static void
mpbios_unmap(struct mp_map *handle)
{
	pmap_kremove(handle->baseva, handle->vsize);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, handle->baseva, handle->vsize, UVM_KMF_VAONLY);
}

/*
 * Look for an Intel MP spec table, indicating SMP capable hardware.
 */
int
mpbios_probe(device_t self)
{
	paddr_t  	ebda, memtop;

	paddr_t		cthpa;
	int		cthlen;
	const uint8_t	*mpbios_page;
	int 		scan_loc;

	struct		mp_map t;

	/* If MP is disabled, don't use MPBIOS or the ioapics. */
	if ((boothowto & RB_MD1) != 0)
		return 0;

	/* see if EBDA exists */

	mpbios_page = mpbios_map(0, PAGE_SIZE, &t);

	ebda = *(const uint16_t *)(&mpbios_page[0x40e]);
	ebda <<= 4;

	memtop = *(const uint16_t *)(&mpbios_page[0x413]);
	memtop <<= 10;

	mpbios_page = NULL;
	mpbios_unmap(&t);

	scan_loc = 0;

	if (ebda && ebda < IOM_BEGIN ) {
		mp_fps = mpbios_search(self, ebda, 1024, &mp_fp_map);
		if (mp_fps != NULL)
			goto found;
	}

	scan_loc = 1;

	if (memtop && memtop <= IOM_BEGIN ) {
		mp_fps = mpbios_search(self, memtop - 1024, 1024, &mp_fp_map);
		if (mp_fps != NULL)
			goto found;
	}

	scan_loc = 2;

	mp_fps = mpbios_search(self, BIOS_BASE, BIOS_COUNT, &mp_fp_map);
	if (mp_fps != NULL)
		goto found;

	/* nothing found */
	return 0;

 found:
	if (mp_verbose)
		aprint_verbose_dev(self,
		    "MP floating pointer found in %s at 0x%jx\n",
		    loc_where[scan_loc], (uintmax_t)mp_fp_map.pa);

	if (mp_fps->pap == 0) {
		if (mp_fps->mpfb1 == 0) {
			aprint_error_dev(self, "MP fps invalid: "
			    "no default config and no configuration table\n");

			goto err;
		}
		return 10;
	}

	cthpa = mp_fps->pap;

	mp_cth = mpbios_map (cthpa, sizeof (*mp_cth), &mp_cfg_table_map);
	cthlen = mp_cth->base_len;
	mpbios_unmap(&mp_cfg_table_map);

	mp_cth = mpbios_map (cthpa, cthlen, &mp_cfg_table_map);

	if (mp_verbose)
		aprint_verbose_dev(self,
		    "MP config table at 0x%jx, %d bytes long\n",
		    (uintmax_t)cthpa, cthlen);

	if (mp_cth->signature != MP_CT_SIG) {
		aprint_error_dev(self, "MP signature mismatch (%x vs %x)\n",
		    MP_CT_SIG, mp_cth->signature);
		goto err;
	}

	if (mpbios_cksum(mp_cth, cthlen)) {
		aprint_error_dev(self,
		    "MP Configuration Table checksum mismatch\n");
		goto err;
	}
	return 10;
 err:
	if (mp_fps) {
		mp_fps = NULL;
		mpbios_unmap(&mp_fp_map);
	}
	if (mp_cth) {
		mp_cth = NULL;
		mpbios_unmap(&mp_cfg_table_map);
	}
	return 0;
}


/*
 * Simple byte checksum used on config tables.
 */

inline static int
mpbios_cksum(const void *start, int len)
{
	unsigned char res=0;
	const char *p = start;
	const char *end = p + len;

	while (p < end)
		res += *p++;

	return res;
}


/*
 * Look for the MP floating pointer signature in the given physical
 * address range.
 *
 * We map the memory, scan through it, and unmap it.
 * If we find it, remap the floating pointer structure and return it.
 */

const void *
mpbios_search(device_t self, paddr_t start, int count,
	      struct mp_map *map)
{
	struct mp_map t;

	int i, len;
	const struct mpbios_fps *m;
	int end = count - sizeof(*m);
	const uint8_t *base = mpbios_map (start, count, &t);

	if (mp_verbose)
		aprint_verbose_dev(self,
		    "scanning 0x%jx to 0x%jx for MP signature\n",
		    (uintmax_t)start, (uintmax_t)(start+count-sizeof(*m)));

	for (i = 0; i <= end; i += 4) {
		m = (const struct mpbios_fps *)&base[i];

		if ((m->signature == MP_FP_SIG) &&
		    ((len = m->length << 4) != 0) &&
		    mpbios_cksum(m, (m->length << 4)) == 0) {
			mpbios_unmap (&t);

			return mpbios_map(start + i, len, map);
		}
	}
	mpbios_unmap(&t);

	return 0;
}

/*
 * MP configuration table parsing.
 */

static struct mpbios_baseentry mp_conf[] =
{
	{0, 20, 0, "cpu"},
	{1, 8, 0, "bus"},
	{2, 8, 0, "ioapic"},
	{3, 8, 0, "ioint"},
	{4, 8, 0, "lint"},
};

static struct mp_bus extint_bus = {
	"ExtINT",
	-1,
	mp_print_special_intr,
	mp_cfg_special_intr,
	NULL, 0, 0, NULL, 0
};
static struct mp_bus smi_bus = {
	"SMI",
	-1,
	mp_print_special_intr,
	mp_cfg_special_intr,
	NULL, 0, 0, NULL, 0
};
static struct mp_bus nmi_bus = {
	"NMI",
	-1,
	mp_print_special_intr,
	mp_cfg_special_intr,
	NULL, 0, 0, NULL, 0
};

/*
 * MP default configuration tables (acc. MP Specification Version 1.4)
 */

/*
 * Default configurations always feature two processors and the local APIC IDs
 * are assigned consecutively by hardware starting from zero (sec. 5). We set
 * I/O APIC ID to 2.
 */
#define DFLT_IOAPIC_ID	2

#define ELCR_INV	0x1
#define MCA_INV		0x2
#define IRQ_VAR		0x4
#define INTIN0_NC	0x8

static const struct dflt_conf_entry dflt_conf_tab[7] = {
	/*
	 * Assume all systems with EISA and (modern) PCI chipsets have ELCRs
	 * (to control external interrupt-level inverters). MCA systems must
	 * use fixed inverters for INTIN1-INTIN15 (table 5-1; sec. 5.3.2).
	 */
	{{"ISA   ", NULL    }, 0                  },	/* default config 1 */
	{{"EISA  ", NULL    }, ELCR_INV | IRQ_VAR },	/* default config 2 */
	{{"EISA  ", NULL    }, ELCR_INV           },	/* default config 3 */
	{{"MCA   ", NULL    }, MCA_INV            },	/* default config 4 */
	{{"ISA   ", "PCI   "}, ELCR_INV           },	/* default config 5 */
	{{"EISA  ", "PCI   "}, ELCR_INV           },	/* default config 6 */
	{{"MCA   ", "PCI   "}, MCA_INV | INTIN0_NC}	/* default config 7 */
};

#define _ (-1) /* INTINx not connected (to a bus interrupt) */

static const int dflt_bus_irq_tab[2][16] = {
	/* Default configs 1,3-7 connect INTIN1-INTIN15 to bus interrupts. */
	{_, 1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
	/*
	 * Default config 2 connects neither timer IRQ0 nor DMA chaining to
	 * INTIN2 and INTIN13 (sec. 5.3).
	 */
	{_, 1, _, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,  _, 14, 15}
};

#undef _

static const uint8_t dflt_lint_tab[2] = {
	MPS_INTTYPE_ExtINT, MPS_INTTYPE_NMI
};


/*
 * 1st pass on BIOS's Intel MP specification table.
 *
 * initializes:
 *	mp_ncpus = 1
 *
 * determines:
 *	cpu_apic_address (common to all CPUs)
 *	ioapic_address[N]
 *	mp_naps
 *	mp_nbus
 *	mp_napics
 *	nintrs
 */
void
mpbios_scan(device_t self, int *ncpup)
{
	const uint8_t 	*position, *end;
	size_t          i;
	int		count;
	int		type;
	int		intr_cnt, cur_intr;
#if NLAPIC > 0
	paddr_t		lapic_base;
#endif
	const struct dflt_conf_entry *dflt_conf;
	const int *dflt_bus_irq;
	const struct mpbios_int *iep;
	struct mpbios_int ie;

	aprint_normal_dev(self, "Intel MP Specification ");

	switch (mp_fps->spec_rev) {
	case 1:
		aprint_normal("(Version 1.1)");
		break;
	case 4:
		aprint_normal("(Version 1.4)");
		break;
	default:
		aprint_normal("(unrecognized rev %d)", mp_fps->spec_rev);
	}

	/*
	 * looks like we've got a MP system.  start setting up
	 * infrastructure..
	 * XXX is this the right place??
	 */

#if NACPICA > 0
	if (mpacpi_ncpu == 0) {
#endif
#if NLAPIC > 0
		lapic_base = LAPIC_BASE;
		if (mp_cth != NULL)
			lapic_base = (paddr_t)mp_cth->apic_address;

		lapic_boot_init(lapic_base);
#endif
#if NACPICA > 0
	}
#endif

	/* check for use of 'default' configuration */
	if (mp_fps->mpfb1 != 0) {

		if (mp_fps->mpfb1 > __arraycount(dflt_conf_tab))
			panic("Unsupported MP default configuration %d\n",
			      mp_fps->mpfb1);

		aprint_normal("\n");
		aprint_normal_dev(self, "MP default configuration %d\n",
		    mp_fps->mpfb1);

		dflt_conf = &dflt_conf_tab[mp_fps->mpfb1 - 1];
		dflt_bus_irq =
		    dflt_bus_irq_tab[(dflt_conf->flags & IRQ_VAR) != 0];

#if NACPICA > 0
		if (mpacpi_ncpu == 0)
#endif
			mpbios_dflt_conf_cpu(self);

#if NACPICA > 0
		if (mpacpi_nioapic == 0)
#endif
			mpbios_dflt_conf_ioapic(self);

		/*
		 * Walk the table once, counting items.
		 */
		mp_nbus = 0;
		for (i = 0; i < __arraycount(dflt_conf->bus_type); i++) {
			if (dflt_conf->bus_type[i] != NULL)
				mp_nbus++;
		}
		KASSERT(mp_nbus != 0);

		mp_busses = kmem_zalloc(sizeof(struct mp_bus) * mp_nbus,
		                        KM_SLEEP);
		KASSERT(mp_busses != NULL);

		/* INTIN0 */
		intr_cnt = (dflt_conf->flags & INTIN0_NC) ? 0 : 1;
		/* INTINx */
		for (i = 0; i < __arraycount(dflt_bus_irq_tab[0]); i++) {
			if (dflt_bus_irq[i] >= 0)
				intr_cnt++;
		}
		KASSERT(intr_cnt != 0);

		/* LINTINx */
		for (i = 0; i < __arraycount(dflt_lint_tab); i++)
			intr_cnt++;

		mp_intrs = kmem_zalloc(sizeof(struct mp_intr_map) * intr_cnt,
		                       KM_SLEEP);
		KASSERT(mp_intrs != NULL);
		mp_nintr = intr_cnt;

		/*
		 * Re-walk the table, recording info of interest.
		 */
		mpbios_dflt_conf_bus(self, dflt_conf);
		mpbios_dflt_conf_int(self, dflt_conf, dflt_bus_irq);
	} else {
		/*
		 * should not happen; mp_probe returns 0 in this case,
		 * but..
		 */
		if (mp_cth == NULL)
			panic ("mpbios_scan: no config (can't happen?)");

		printf(" (%8.8s %12.12s)\n",
		    mp_cth->oem_id, mp_cth->product_id);

		/*
		 * Walk the table once, counting items
		 */
		position = (const uint8_t *)(mp_cth);
		end = position + mp_cth->base_len;
		position += sizeof(*mp_cth);

		count = mp_cth->entry_count;
		intr_cnt = 0;

		while ((count--) && (position < end)) {
			type = *position;
			if (type >= MPS_MCT_NTYPES) {
				aprint_error_dev(self, "unknown entry type %x"
				    " in MP config table\n", type);
				break;
			}
			mp_conf[type].count++;
			if (type == MPS_MCT_BUS) {
				const struct mpbios_bus *bp =
				    (const struct mpbios_bus *)position;
				if (bp->bus_id >= mp_nbus)
					mp_nbus = bp->bus_id + 1;
			}
			/*
			 * Count actual interrupt instances.
			 * dst_apic_id of MPS_ALL_APICS means "wired to all
			 * apics of this type".
			 */
			if (type == MPS_MCT_IOINT) {
				iep = (const struct mpbios_int *)position;
				if (iep->dst_apic_id == MPS_ALL_APICS)
					intr_cnt +=
					    mp_conf[MPS_MCT_IOAPIC].count;
				else
					intr_cnt++;
			} else if (type == MPS_MCT_LINT)
				intr_cnt++;
			position += mp_conf[type].length;
		}

		mp_busses = kmem_zalloc(sizeof(struct mp_bus)*mp_nbus,
		    KM_SLEEP);
		KASSERT(mp_busses != NULL);
		mp_intrs = kmem_zalloc(sizeof(struct mp_intr_map)*intr_cnt,
		    KM_SLEEP);
		KASSERT(mp_intrs != NULL);
		mp_nintr = intr_cnt;

		/* re-walk the table, recording info of interest */
		position = (const uint8_t *)mp_cth + sizeof(*mp_cth);
		count = mp_cth->entry_count;
		cur_intr = 0;

		while ((count--) && (position < end)) {
			switch (type = *position) {
			case MPS_MCT_CPU:
#if NACPICA > 0
				/* ACPI has done this for us */
				if (mpacpi_ncpu)
					break;
#endif
				mpbios_cpu(position, self);
				break;
			case MPS_MCT_BUS:
				mpbios_bus(position, self);
				break;
			case MPS_MCT_IOAPIC:
#if NACPICA > 0
				/* ACPI has done this for us */
				if (mpacpi_nioapic)
					break;
#endif
				mpbios_ioapic(position, self);
				break;
			case MPS_MCT_IOINT:
				iep = (const struct mpbios_int *)position;
				ie = *iep;
				if (iep->dst_apic_id == MPS_ALL_APICS) {
#if NIOAPIC > 0
					struct ioapic_softc *sc;
					for (sc = ioapics ; sc != NULL;
					     sc = sc->sc_next) {
						ie.dst_apic_id = sc->sc_pic.pic_apicid;
						mpbios_int((char *)&ie, type,
						    &mp_intrs[cur_intr++]);
					}
#endif
				} else {
					mpbios_int(position, type,
					    &mp_intrs[cur_intr++]);
				}
				break;
			case MPS_MCT_LINT:
				mpbios_int(position, type,
				    &mp_intrs[cur_intr]);
				cur_intr++;
				break;
			default:
				aprint_error_dev(self, "unknown entry type %x"
				    " in MP config table\n", type);
				/* NOTREACHED */
				return;
			}

			position += mp_conf[type].length;
		}
		if (mp_verbose && mp_cth->ext_len)
			aprint_verbose_dev(self, "MP WARNING: %d bytes of"
			    " extended entries not examined\n",
			    mp_cth->ext_len);
	}
	/* Clean up. */
	mp_fps = NULL;
	mpbios_unmap (&mp_fp_map);
	if (mp_cth != NULL) {
		mp_cth = NULL;
		mpbios_unmap (&mp_cfg_table_map);
	}
	mpbios_scanned = 1;

	*ncpup = mpbios_ncpu;
}

static void
mpbios_cpu(const uint8_t *ent, device_t self)
{
	const struct mpbios_proc *entry = (const struct mpbios_proc *)ent;
	struct cpu_attach_args caa;
	int locs[CPUBUSCF_NLOCS];

	/* XXX move this into the CPU attachment goo. */
	/* check for usability */
	if (!(entry->cpu_flags & PROCENTRY_FLAG_EN))
		return;

	mpbios_ncpu++;

	/* check for BSP flag */
	if (entry->cpu_flags & PROCENTRY_FLAG_BP)
		caa.cpu_role = CPU_ROLE_BP;
	else
		caa.cpu_role = CPU_ROLE_AP;

	caa.cpu_id = entry->apic_id;
	caa.cpu_number = entry->apic_id;
	caa.cpu_func = &mp_cpu_funcs;
	locs[CPUBUSCF_APID] = caa.cpu_number;

	config_found_sm_loc(self, "cpubus", locs, &caa, mp_cpuprint,
			    config_stdsubmatch);
}

static void
mpbios_dflt_conf_cpu(device_t self)
{
	struct mpbios_proc mpp;

	/* mpp.type and mpp.apic_version are irrelevant for mpbios_cpu(). */
	/*
	 * Default configurations always feature two processors and the local
	 * APIC IDs are assigned consecutively by hardware starting from zero
	 * (sec. 5). Just determine the BSP (APIC ID).
	 */
	mpp.apic_id = cpu_info_primary.ci_cpuid;
	mpp.cpu_flags = PROCENTRY_FLAG_EN | PROCENTRY_FLAG_BP;
	mpbios_cpu((uint8_t *)&mpp, self);

	mpp.apic_id = 1 - cpu_info_primary.ci_cpuid;
	mpp.cpu_flags = PROCENTRY_FLAG_EN;
	mpbios_cpu((uint8_t *)&mpp, self);
}

static void
mpbios_dflt_conf_bus(device_t self, const struct dflt_conf_entry *dflt_conf)
{
	struct mpbios_bus mpb;
	size_t i;

	/* mpb.type is irrelevant for mpbios_bus(). */
	mpb.bus_id = 0;
	for (i = 0; i < __arraycount(dflt_conf->bus_type); i++) {
		if (dflt_conf->bus_type[i] != NULL) {
			memcpy(mpb.bus_type, dflt_conf->bus_type[i], 6);
			mpbios_bus((u_int8_t *)&mpb, self);
			mpb.bus_id++;
		}
	}
}

static void
mpbios_dflt_conf_ioapic(device_t self)
{
	struct mpbios_ioapic mpio;

	/* mpio.type is irrelevant for mpbios_ioapic(). */
	mpio.apic_id = DFLT_IOAPIC_ID;
	/* XXX Let ioapic driver read real APIC version... */
	mpio.apic_version = 0;
	mpio.apic_flags = IOAPICENTRY_FLAG_EN;
	mpio.apic_address = (uint32_t)IOAPIC_BASE_DEFAULT;
	mpbios_ioapic((uint8_t *)&mpio, self);
}

static void
mpbios_dflt_conf_int(device_t self, const struct dflt_conf_entry *dflt_conf,
                     const int *dflt_bus_irq)
{
	struct   mpbios_int mpi;
	size_t   i;
	int      cur_intr;
	uint16_t level_inv;

	cur_intr = 0;
	/*
	 * INTIN0
	 */
	/*
	 * 8259A INTR is connected to INTIN0 for default configs 1-6, but not
	 * for default config 7 (sec. 5.3).
	 */
	if ((dflt_conf->flags & INTIN0_NC) == 0) {
		/* mpi.type is irrelevant for mpbios_int(). */
		mpi.int_type = MPS_INTTYPE_ExtINT;
		mpi.int_flags = 0;
		mpi.src_bus_id = 0;
		mpi.src_bus_irq = 0;
		mpi.dst_apic_id = DFLT_IOAPIC_ID;
		mpi.dst_apic_int = 0;
		mpbios_int((u_int8_t *)&mpi, MPS_MCT_IOINT,
		           &mp_intrs[cur_intr++]);
	}

	/*
	 * INTINx
	 */
	/* mpi.type is irrelevant for mpbios_int(). */
	mpi.int_type = MPS_INTTYPE_INT;
	/*
	 * PCI interrupt lines appear as (E)ISA interrupt lines/on bus 0
	 * (sec. 5.2).
	 */
	mpi.src_bus_id = 0;
	mpi.dst_apic_id = DFLT_IOAPIC_ID;
	/*
	 * Compliant systems must convert active-low, level-triggered
	 * interrupts to active-high by external inverters before INTINx
	 *(table 5-1; sec. 5.3.2).
	 */
	if (dflt_conf->flags & ELCR_INV) {
#ifdef X86_MPBIOS_SUPPORT_EISA
		/* Systems with ELCRs use them to control the inverters. */
		level_inv = ((uint16_t)inb(ELCR1) << 8) | inb(ELCR0);
#else
		level_inv = 0;
#endif
	} else if (dflt_conf->flags & MCA_INV) {
		/* MCA systems have fixed inverters. */
		level_inv = 0xffffU;
	} else
		level_inv = 0;

	for (i = 0; i < __arraycount(dflt_bus_irq_tab[0]); i++) {
		if (dflt_bus_irq[i] >= 0) {
			mpi.src_bus_irq = (uint8_t)dflt_bus_irq[i];
			if (level_inv & (1U << mpi.src_bus_irq))
				mpi.int_flags = (MPS_INTTR_LEVEL << 2)
				                | MPS_INTPO_ACTHI;
			else
				mpi.int_flags = 0; /* conforms to bus spec. */
			mpi.dst_apic_int = (uint8_t)i;
			mpbios_int((u_int8_t *)&mpi, MPS_MCT_IOINT,
			           &mp_intrs[cur_intr++]);
		}
	}

	/*
	 * LINTINx
	 */
	/* mpi.type is irrelevant for mpbios_int(). */
	mpi.int_flags = 0;
	mpi.src_bus_id = 0;
	mpi.src_bus_irq = 0;
	mpi.dst_apic_id = MPS_ALL_APICS;
	for (i = 0; i < __arraycount(dflt_lint_tab); i++) {
		mpi.int_type = dflt_lint_tab[i];
		mpi.dst_apic_int = (uint8_t)i;
		mpbios_int((u_int8_t *)&mpi, MPS_MCT_LINT,
		           &mp_intrs[cur_intr++]);
	}
}

/*
 * The following functions conspire to compute base ioapic redirection
 * table entry for a given interrupt line.
 *
 * Fill in: trigger mode, polarity, and possibly delivery mode.
 */
static void
mp_cfg_special_intr(const struct mpbios_int *entry, uint32_t *redir)
{

	/*
	 * All of these require edge triggered, zero vector,
	 * appropriate delivery mode.
	 * see page 13 of the 82093AA datasheet.
	 */
	*redir &= ~IOAPIC_REDLO_DEL_MASK;
	*redir &= ~IOAPIC_REDLO_VECTOR_MASK;
	*redir &= ~IOAPIC_REDLO_LEVEL;

	switch (entry->int_type) {
	case MPS_INTTYPE_NMI:
		*redir |= (IOAPIC_REDLO_DEL_NMI<<IOAPIC_REDLO_DEL_SHIFT);
		break;

	case MPS_INTTYPE_SMI:
		*redir |= (IOAPIC_REDLO_DEL_SMI<<IOAPIC_REDLO_DEL_SHIFT);
		break;

	case MPS_INTTYPE_ExtINT:
		/*
		 * We are using the ioapic in "native" mode.
		 * This indicates where the 8259 is wired to the ioapic
		 * and/or local apic..
		 */
		*redir |= (IOAPIC_REDLO_DEL_EXTINT<<IOAPIC_REDLO_DEL_SHIFT);
		*redir |= (IOAPIC_REDLO_MASK);
		break;
	}
}

/* XXX too much duplicated code here. */

static void
mp_cfg_pci_intr(const struct mpbios_int *entry, uint32_t *redir)
{
	int mpspo = entry->int_flags & 0x03; /* XXX magic */
	int mpstrig = (entry->int_flags >> 2) & 0x03; /* XXX magic */

	*redir &= ~IOAPIC_REDLO_DEL_MASK;
	switch (mpspo) {
	case MPS_INTPO_ACTHI:
		*redir &= ~IOAPIC_REDLO_ACTLO;
		break;
	case MPS_INTPO_DEF:
	case MPS_INTPO_ACTLO:
		*redir |= IOAPIC_REDLO_ACTLO;
		break;
	default:
		panic("unknown MPS interrupt polarity %d", mpspo);
	}

	if (entry->int_type != MPS_INTTYPE_INT) {
		mp_cfg_special_intr(entry, redir);
		return;
	}
	*redir |= (IOAPIC_REDLO_DEL_FIXED<<IOAPIC_REDLO_DEL_SHIFT);

	switch (mpstrig) {
	case MPS_INTTR_DEF:
	case MPS_INTTR_LEVEL:
		*redir |= IOAPIC_REDLO_LEVEL;
		break;
	case MPS_INTTR_EDGE:
		*redir &= ~IOAPIC_REDLO_LEVEL;
		break;
	default:
		panic("unknown MPS interrupt trigger %d", mpstrig);
	}
}

#ifdef X86_MPBIOS_SUPPORT_EISA
static void
mp_cfg_eisa_intr(const struct mpbios_int *entry, uint32_t *redir)
{
	int mpspo = entry->int_flags & 0x03; /* XXX magic */
	int mpstrig = (entry->int_flags >> 2) & 0x03; /* XXX magic */

	*redir &= ~IOAPIC_REDLO_DEL_MASK;
	switch (mpspo) {
	case MPS_INTPO_DEF:
	case MPS_INTPO_ACTHI:
		*redir &= ~IOAPIC_REDLO_ACTLO;
		break;
	case MPS_INTPO_ACTLO:
		*redir |= IOAPIC_REDLO_ACTLO;
		break;
	default:
		panic("unknown MPS interrupt polarity %d", mpspo);
	}

	if (entry->int_type != MPS_INTTYPE_INT) {
		mp_cfg_special_intr(entry, redir);
		return;
	}
	*redir |= (IOAPIC_REDLO_DEL_FIXED << IOAPIC_REDLO_DEL_SHIFT);

	switch (mpstrig) {
	case MPS_INTTR_LEVEL:
		*redir |= IOAPIC_REDLO_LEVEL;
		break;
	case MPS_INTTR_EDGE:
		*redir &= ~IOAPIC_REDLO_LEVEL;
		break;
	case MPS_INTTR_DEF:
		/*
		 * Set "default" setting based on ELCR value snagged
		 * earlier.
		 */
		if (mp_busses[entry->src_bus_id].mb_data &
		    (1 << entry->src_bus_irq)) {
			*redir |= IOAPIC_REDLO_LEVEL;
		} else {
			*redir &= ~IOAPIC_REDLO_LEVEL;
		}
		break;
	default:
		panic("unknown MPS interrupt trigger %d", mpstrig);
	}
}
#endif


static void
mp_cfg_isa_intr(const struct mpbios_int *entry, uint32_t *redir)
{
	int mpspo = entry->int_flags & 0x03; /* XXX magic */
	int mpstrig = (entry->int_flags >> 2) & 0x03; /* XXX magic */

	*redir &= ~IOAPIC_REDLO_DEL_MASK;
	switch (mpspo) {
	case MPS_INTPO_DEF:
	case MPS_INTPO_ACTHI:
		*redir &= ~IOAPIC_REDLO_ACTLO;
		break;
	case MPS_INTPO_ACTLO:
		*redir |= IOAPIC_REDLO_ACTLO;
		break;
	default:
		panic("unknown MPS interrupt polarity %d", mpspo);
	}

	if (entry->int_type != MPS_INTTYPE_INT) {
		mp_cfg_special_intr(entry, redir);
		return;
	}
	*redir |= (IOAPIC_REDLO_DEL_FIXED << IOAPIC_REDLO_DEL_SHIFT);

	switch (mpstrig) {
	case MPS_INTTR_LEVEL:
		*redir |= IOAPIC_REDLO_LEVEL;
		break;
	case MPS_INTTR_DEF:
	case MPS_INTTR_EDGE:
		*redir &= ~IOAPIC_REDLO_LEVEL;
		break;
	default:
		panic("unknown MPS interrupt trigger %d", mpstrig);
	}
}


static void
mp_print_special_intr(int intr)
{
}

static void
mp_print_pci_intr(int intr)
{
	printf(" device %d INT_%c", (intr>>2)&0x1f, 'A' + (intr & 0x3));
}

static void
mp_print_isa_intr(int intr)
{
	printf(" irq %d", intr);
}

#ifdef X86_MPBIOS_SUPPORT_EISA
static void
mp_print_eisa_intr(int intr)
{
	printf(" EISA irq %d", intr);
}
#endif



#define TAB_UNIT	4
#define TAB_ROUND(a)	_TAB_ROUND(a, TAB_UNIT)

#define _TAB_ROUND(a,u)	(((a) + (u - 1)) & ~(u-1))
#define EXTEND_TAB(a,u)	(!(_TAB_ROUND(a,u) == _TAB_ROUND((a+1),u)))

static void
mpbios_bus(const uint8_t *ent, device_t self)
{
	const struct mpbios_bus *entry = (const struct mpbios_bus *)ent;
	int bus_id = entry->bus_id;

	aprint_verbose("mpbios: bus %d is type %6.6s\n", bus_id,
	    entry->bus_type);

#ifdef DIAGNOSTIC
	/*
	 * This "should not happen" unless the table changes out
	 * from underneath us
	 */
	if (bus_id >= mp_nbus) {
		panic("mpbios: bus number %d out of range?? (type %6.6s)\n",
		    bus_id, entry->bus_type);
	}
#endif

	mp_busses[bus_id].mb_intrs = NULL;

	if (memcmp(entry->bus_type, "PCI   ", 6) == 0) {
		mp_busses[bus_id].mb_name = "pci";
		mp_busses[bus_id].mb_idx = bus_id;
		mp_busses[bus_id].mb_intr_print = mp_print_pci_intr;
		mp_busses[bus_id].mb_intr_cfg = mp_cfg_pci_intr;
#ifdef X86_MPBIOS_SUPPORT_EISA
	} else if (memcmp(entry->bus_type, "EISA  ", 6) == 0) {
		mp_busses[bus_id].mb_name = "eisa";
		mp_busses[bus_id].mb_idx = bus_id;
		mp_busses[bus_id].mb_intr_print = mp_print_eisa_intr;
		mp_busses[bus_id].mb_intr_cfg = mp_cfg_eisa_intr;

		mp_busses[bus_id].mb_data = inb(ELCR0) | (inb(ELCR1) << 8);

		if (mp_eisa_bus != -1)
			aprint_error("oops: multiple isa busses?\n");
		else
			mp_eisa_bus = bus_id;
#endif

	} else if (memcmp(entry->bus_type, "ISA   ", 6) == 0) {
		mp_busses[bus_id].mb_name = "isa";
		mp_busses[bus_id].mb_idx = 0; /* XXX */
		mp_busses[bus_id].mb_intr_print = mp_print_isa_intr;
		mp_busses[bus_id].mb_intr_cfg = mp_cfg_isa_intr;
		if (mp_isa_bus != -1)
			printf("oops: multiple isa busses?\n");
		else
			mp_isa_bus = bus_id;
	} else
		aprint_error_dev(self, "unsupported bus type %6.6s\n",
		    entry->bus_type);
}


static void
mpbios_ioapic(const uint8_t *ent, device_t self)
{
	const struct mpbios_ioapic *entry = (const struct mpbios_ioapic *)ent;

	/* XXX let flags checking happen in ioapic driver.. */
	if (!(entry->apic_flags & IOAPICENTRY_FLAG_EN))
		return;

	mpbios_nioapic++;

#if NIOAPIC > 0
	{
	int locs[IOAPICBUSCF_NLOCS];
	struct apic_attach_args aaa;

	aaa.apic_id = entry->apic_id;
	aaa.apic_version = entry->apic_version;
	aaa.apic_address = (paddr_t)entry->apic_address;
	aaa.apic_vecbase = -1;
	aaa.flags = (mp_fps->mpfb2 & 0x80) ? IOAPIC_PICMODE : IOAPIC_VWIRE;
	locs[IOAPICBUSCF_APID] = aaa.apic_id;

	config_found_sm_loc(self, "ioapicbus", locs, &aaa, mp_ioapicprint,
			    config_stdsubmatch);
	}
#endif
}

static const char inttype_fmt[] = "\177\020"
		"f\0\2type\0" "=\1NMI\0" "=\2SMI\0" "=\3ExtINT\0";

static const char flagtype_fmt[] = "\177\020"
		"f\0\2pol\0" "=\1Act Hi\0" "=\3Act Lo\0"
		"f\2\2trig\0" "=\1Edge\0" "=\3Level\0";

static void
mpbios_int(const uint8_t *ent, int enttype, struct mp_intr_map *mpi)
{
	const struct mpbios_int *entry = (const struct mpbios_int *)ent;
	struct ioapic_softc *sc = NULL;
	struct pic *sc2;

	struct mp_intr_map *altmpi;
	struct mp_bus *mpb;

	uint32_t id = entry->dst_apic_id;
	uint32_t pin = entry->dst_apic_int;
	uint32_t bus = entry->src_bus_id;
	uint32_t dev = entry->src_bus_irq;
	uint32_t type = entry->int_type;
	uint32_t flags = entry->int_flags;

	switch (type) {
	case MPS_INTTYPE_INT:
		mpb = &(mp_busses[bus]);
		break;
	case MPS_INTTYPE_ExtINT:
		mpb = &extint_bus;
		break;
	case MPS_INTTYPE_SMI:
		mpb = &smi_bus;
		break;
	case MPS_INTTYPE_NMI:
		mpb = &nmi_bus;
		break;
	default:
		panic("unknown MPS interrupt type %d", entry->int_type);
	}

	mpi->next = mpb->mb_intrs;
	mpb->mb_intrs = mpi;
	mpi->bus = mpb;
	mpi->bus_pin = dev;
	mpi->global_int = -1;

	mpi->type = type;
	mpi->flags = flags;
	mpi->redir = 0;
	if (mpb->mb_intr_cfg == NULL) {
		printf("mpbios: can't find bus %d for apic %d pin %d\n",
		    bus, id, pin);
		return;
	}

	(*mpb->mb_intr_cfg)(entry, &mpi->redir);

	if (enttype == MPS_MCT_IOINT) {
#if NIOAPIC > 0
		sc = ioapic_find(id);
#else
		sc = NULL;
#endif
		if (sc == NULL) {
#if NIOAPIC > 0
			/*
			 * If we couldn't find an ioapic by given id, retry to
			 * get it by a pin number.
			 */
			sc = ioapic_find_bybase(pin);
			if (sc == NULL) {
				aprint_error("mpbios: can't find ioapic by"
				    " neither apid(%d) nor pin number(%d)\n",
				    id, pin);
				return;
			}
			aprint_verbose("mpbios: use apid %d instead of %d\n",
			    sc->sc_pic.pic_apicid, id);
			id = sc->sc_pic.pic_apicid;
#else
			aprint_error("mpbios: can't find ioapic %d\n", id);
			return;
#endif
		}

		/*
		 * XXX workaround for broken BIOSs that put the ACPI global
		 * interrupt number in the entry, not the pin number.
		 */
		if (pin >= sc->sc_apic_sz) {
			sc2 = intr_findpic(pin);
			if (sc2 && sc2->pic_ioapic != sc) {
				printf("mpbios: bad pin %d for apic %d\n",
				    pin, id);
				return;
			}
			printf("mpbios: WARNING: pin %d for apic %d too high; "
			       "assuming ACPI global int value\n", pin, id);
			pin -= sc->sc_apic_vecbase;
		}

		mpi->ioapic = (struct pic *)sc;
		mpi->ioapic_pin = pin;

		altmpi = sc->sc_pins[pin].ip_map;

		if (altmpi != NULL) {
			if ((altmpi->type != type) ||
			    (altmpi->flags != flags)) {
				printf("%s: conflicting map entries for pin %d\n",
				    device_xname(sc->sc_dev), pin);
			}
		} else
			sc->sc_pins[pin].ip_map = mpi;
	} else {
		if (pin >= 2)
			printf("pin %d of local apic doesn't exist!\n", pin);
		else {
			mpi->ioapic = NULL;
			mpi->ioapic_pin = pin;
			mpi->cpu_id = id;
		}
	}

	mpi->ioapic_ih = APIC_INT_VIA_APIC |
	    ((id << APIC_INT_APIC_SHIFT) | (pin << APIC_INT_PIN_SHIFT));

	if (mp_verbose) {
		char buf[256];

		printf("%s: int%d attached to %s",
		    sc ? device_xname(sc->sc_dev) : "local apic",
		    pin, mpb->mb_name);

		if (mpb->mb_idx != -1)
			printf("%d", mpb->mb_idx);

		(*(mpb->mb_intr_print))(dev);

		snprintb(buf, sizeof(buf), inttype_fmt, type);
		printf(" (type %s", buf);

		snprintb(buf, sizeof(buf), flagtype_fmt, flags);
		printf(" flags %s)\n", buf);
	}
}

#if NPCI > 0
int
mpbios_pci_attach_hook(device_t parent, device_t self,
		       struct pcibus_attach_args *pba)
{
	struct mp_bus *mpb;

	if (mpbios_scanned == 0)
		return ENOENT;

	if (pba->pba_bus >= mp_isa_bus) {
		intr_add_pcibus(pba);
		return 0;
	}

	mpb = &mp_busses[pba->pba_bus];
	if (mpb->mb_name != NULL) {
		if (strcmp(mpb->mb_name, "pci"))
			return EINVAL;
	} else
		mpb->mb_name = "pci";

	if (mp_verbose)
		printf("\n%s: added to list as bus %d", device_xname(parent),
		    pba->pba_bus);

	mpb->mb_dev = self;
	mpb->mb_pci_bridge_tag = pba->pba_bridgetag;
	mpb->mb_pci_chipset_tag = pba->pba_pc;
	return 0;
}
#endif
