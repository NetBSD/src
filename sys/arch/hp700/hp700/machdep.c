/*	$NetBSD: machdep.c,v 1.1 2002/06/06 19:48:06 fredette Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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

/*	$OpenBSD: machdep.c,v 1.40 2001/09/19 20:50:56 mickey Exp $	*/

/*
 * Copyright (c) 1999-2000 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/sysctl.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/extent.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_page.h>
#include <uvm/uvm.h>

#include <dev/cons.h>

#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/cpufunc.h>
#include <machine/autoconf.h>
#include <machine/kcore.h>

#ifdef COMPAT_HPUX
#include <compat/hpux/hpux.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include <hp700/hp700/intr.h>
#include <hp700/hp700/machdep.h>
#include <hp700/hp700/pim.h>
#include <hp700/dev/cpudevs.h>

/*
 * Different kinds of flags used throughout the kernel.
 */
caddr_t msgbufaddr;

/*
 * cache configuration, for most machines is the same
 * numbers, so it makes sense to do defines w/ numbers depending
 * on cofigured cpu types in the kernel
 */
int icache_stride, icache_line_mask;
int dcache_stride, dcache_line_mask;

/*
 * things to not kill
 */
volatile u_int8_t *machine_ledaddr;
int machine_ledword, machine_leds;

/*
 * This flag is nonzero iff page zero is mapped.
 * It is initialized to 1, because before we go
 * virtual, page zero *is* available.  It is set
 * to zero right before we go virtual.
 */
static int pagezero_mapped = 1;

/*
 * CPU params (should be the same for all cpus in the system)
 */
struct pdc_cache pdc_cache PDC_ALIGNMENT;
struct pdc_btlb pdc_btlb PDC_ALIGNMENT;

/*
 * A record of inserted BTLB mappings.
 */
struct btlb_entry {
	pa_space_t btlb_entry_va_space;
	vaddr_t	btlb_entry_va_frame;
	paddr_t btlb_entry_pa_frame;
	vsize_t btlb_entry_frames;
	u_int btlb_entry_tlbprot;
	int btlb_entry_num;
} btlb_entries[32]; /* XXX assumed to be big enough */

	/* w/ a little deviation should be the same for all installed cpus */
u_int	cpu_ticksnum, cpu_ticksdenom, cpu_hzticks;

	/* exported info */
char	machine[] = MACHINE;
char	cpu_model[128];
enum hppa_cpu_type cpu_type;
const char *cpu_typename;
#ifdef COMPAT_HPUX
int	cpu_model_hpux;	/* contains HPUX_SYSCONF_CPU* kind of value */
#endif

/*
 * exported methods for cpus
 */
int (*cpu_desidhash) __P((void));
int (*cpu_hpt_init) __P((vaddr_t hpt, vsize_t hptsize));
int (*cpu_ibtlb_ins) __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
	    vsize_t sz, u_int prot));
int (*cpu_dbtlb_ins) __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
	    vsize_t sz, u_int prot));

dev_t	bootdev;
int	totalphysmem, resvmem, physmem, esym;

/*
 * Things for MI glue to stick on.
 */
struct user *proc0paddr;
long mem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct extent *hppa_ex;

/* Virtual page frame for /dev/mem (see mem.c) */
vaddr_t vmmap;

/*
 * XXX fredette - a bus_dma hack, brought on by our use of a 
 * BTLB entry to map the kernel, I think.
 */
#define	DMA24_SIZE_MAX	(128 * 1024)
#define	DMA24_SIZE_MIN	(32 * 1024)
#define	DMA24_SIZE_STEP	(4 * 1024)
struct extent *dma24_ex;
long dma24_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;


void delay_init __P((void));
static __inline void fall __P((int, int, int, int, int));
void dumpsys __P((void));

/*
 * wide used hardware params
 */
struct pdc_hwtlb pdc_hwtlb PDC_ALIGNMENT;
struct pdc_coproc pdc_coproc PDC_ALIGNMENT;
struct pdc_coherence pdc_coherence PDC_ALIGNMENT;
struct pdc_spidb pdc_spidbits PDC_ALIGNMENT;
struct pdc_pim pdc_pim PDC_ALIGNMENT;

/*
 * Whatever CPU types we support
 */
extern const u_int itlb_x[], dtlb_x[], dtlbna_x[], tlbd_x[];
extern const u_int itlb_s[], dtlb_s[], dtlbna_s[], tlbd_s[];
extern const u_int itlb_t[], dtlb_t[], dtlbna_t[], tlbd_t[];
extern const u_int itlb_l[], dtlb_l[], dtlbna_l[], tlbd_l[];
int iibtlb_s __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
    vsize_t sz, u_int prot));
int idbtlb_s __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
    vsize_t sz, u_int prot));
int ibtlb_t __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
    vsize_t sz, u_int prot));
int ibtlb_l __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
    vsize_t sz, u_int prot));
int ibtlb_g __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
    vsize_t sz, u_int prot));
int pbtlb_g __P((int i));
int hpti_l __P((vaddr_t, vsize_t));
int hpti_g __P((vaddr_t, vsize_t));
int desidhash_x __P((void));
int desidhash_s __P((void));
int desidhash_t __P((void));
int desidhash_l __P((void));
int desidhash_g __P((void));
const struct hppa_cpu_typed {
	char name[8];
	enum hppa_cpu_type type;
	int  arch;
	int  features;
	int (*desidhash) __P((void));
	const u_int *itlbh, *dtlbh, *dtlbnah, *tlbdh;
	int (*dbtlbins) __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
	    vsize_t sz, u_int prot));
	int (*ibtlbins) __P((int i, pa_space_t sp, vaddr_t va, paddr_t pa,
	    vsize_t sz, u_int prot));
	int (*btlbprg) __P((int i));
	int (*hptinit) __P((vaddr_t hpt, vsize_t hptsize));
} cpu_types[] = {
#ifdef HP7000_CPU
	{ "PCX",   hpcx,  0x10, 0,
	  desidhash_x, itlb_x, dtlb_x, dtlbna_x, tlbd_x,
	  ibtlb_g, NULL, pbtlb_g},
	{ "PCXS",  hpcxs, 0x11, HPPA_FTRS_BTLBS,
	  desidhash_s, itlb_s, dtlb_s, dtlbna_s, tlbd_s,
	  ibtlb_g, NULL, pbtlb_g},
#endif
#ifdef HP7100_CPU
	{ "PCXT",  hpcxt, 0x11, HPPA_FTRS_BTLBU,
	  desidhash_t, itlb_t, dtlb_t, dtlbna_t, tlbd_t,
	  ibtlb_g, NULL, pbtlb_g},
#endif
#ifdef HP7200_CPU
/* HOW?	{ "PCXT'", hpcxta,0x11, HPPA_FTRS_BTLBU,
	  desidhash_t, itlb_t, dtlb_t, dtlbna_t, tlbd_t,
	  ibtlb_g, NULL, pbtlb_g}, */
#endif
#ifdef HP7100LC_CPU
	{ "PCXL",  hpcxl, 0x11, HPPA_FTRS_BTLBU|HPPA_FTRS_HVT,
	  desidhash_l, itlb_l, dtlb_l, dtlbna_l, tlbd_l,
	  ibtlb_g, NULL, pbtlb_g, hpti_g},
#endif
#ifdef HP7300LC_CPU
/* HOW?	{ "PCXL2", hpcxl2,0x11, HPPA_FTRS_BTLBU|HPPA_FTRS_HVT,
	  desidhash_l, itlb_l, dtlb_l, dtlbna_l, tlbd_l,
	  ibtlb_g, NULL, pbtlb_g, hpti_g}, */
#endif
#ifdef HP8000_CPU
	{ "PCXU",  hpcxu, 0x20, HPPA_FTRS_W32B|HPPA_FTRS_BTLBU|HPPA_FTRS_HVT,
	  desidhash_g, itlb_l, dtlb_l, dtlbna_l, tlbd_l,
	  ibtlb_g, NULL, pbtlb_g, hpti_g},
#endif
#ifdef HP8200_CPU
/* HOW?	{ "PCXU2", hpcxu2,0x20, HPPA_FTRS_W32B|HPPA_FTRS_BTLBU|HPPA_FTRS_HVT,
	  desidhash_g, itlb_l, dtlb_l, dtlbna_l, tlbd_l,
	  ibtlb_g, NULL, pbtlb_g, hpti_g}, */
#endif
#ifdef HP8500_CPU
/* HOW?	{ "PCXW",  hpcxw, 0x20, HPPA_FTRS_W32B|HPPA_FTRS_BTLBU|HPPA_FTRS_HVT,
	  desidhash_g, itlb_l, dtlb_l, dtlbna_l, tlbd_l,
	  ibtlb_g, NULL, pbtlb_g, hpti_g}, */
#endif
#ifdef HP8600_CPU
/* HOW?	{ "PCXW+", hpcxw, 0x20, HPPA_FTRS_W32B|HPPA_FTRS_BTLBU|HPPA_FTRS_HVT,
	  desidhash_g, itlb_l, dtlb_l, dtlbna_l, tlbd_l,
	  ibtlb_g, NULL, pbtlb_g, hpti_g}, */
#endif
	{ "", 0 }
};

void
hppa_init(start)
	paddr_t start;
{
	extern int kernel_text;
	vaddr_t v, vstart, vend;
	register int error;
	int hptsize;	/* size of HPT table if supported */
	int cpu_features = 0;
	int dma24size;
	int sz;
	u_int *p, *q;

#ifdef KGDB
	boothowto |= RB_KDB;	/* go to kgdb early if compiled in. */
#endif

	pdc_init();	/* init PDC iface, so we can call em easy */

	cpu_hzticks = (PAGE0->mem_10msec * 100) / hz;
	delay_init();	/* calculate cpu clock ratio */

	/* cache parameters */
	if ((error = pdc_call((iodcio_t)pdc, 0, PDC_CACHE, PDC_CACHE_DFLT,
	    &pdc_cache)) < 0) {
#ifdef DEBUG
		printf("WARNING: PDC_CACHE error %d\n", error);
#endif
	}

	dcache_line_mask = pdc_cache.dc_conf.cc_line * 16 - 1;
	dcache_stride = pdc_cache.dc_stride;
	icache_line_mask = pdc_cache.ic_conf.cc_line * 16 - 1;
	icache_stride = pdc_cache.ic_stride;

	/* cache coherence params (pbably available for 8k only) */
	error = pdc_call((iodcio_t)pdc, 0, PDC_CACHE, PDC_CACHE_SETCS,
	    &pdc_coherence, 1, 1, 1, 1);
#ifdef DEBUG
	printf ("PDC_CACHE_SETCS: %d, %d, %d, %d (%d)\n",
	    pdc_coherence.ia_cst, pdc_coherence.da_cst,
	    pdc_coherence.ita_cst, pdc_coherence.dta_cst, error);
#endif
	error = pdc_call((iodcio_t)pdc, 0, PDC_CACHE, PDC_CACHE_GETSPIDB,
	    &pdc_spidbits, 0, 0, 0, 0);
	printf("SPID bits: 0x%x, error = %d\n", pdc_spidbits.spidbits, error);

	/* Calculate the OS_HPMC handler checksums. */
	p = &os_hpmc;
	if (pdc_call((iodcio_t)pdc, 0, PDC_INSTR, PDC_INSTR_DFLT, p))
		*p = 0;
	p[7] = ((caddr_t) &os_hpmc_cont_end) - ((caddr_t) &os_hpmc_cont);
	p[6] = (u_int) &os_hpmc_cont;
	p[5] = -(p[0] + p[1] + p[2] + p[3] + p[4] + p[6] + p[7]);
	p = &os_hpmc_cont;
	q = (&os_hpmc_cont_end - 1);
	for(*q = 0; p < q; *q -= *(p++));

	/* BTLB params */
	if ((error = pdc_call((iodcio_t)pdc, 0, PDC_BLOCK_TLB,
	    PDC_BTLB_DEFAULT, &pdc_btlb)) < 0) {
#ifdef DEBUG
		printf("WARNING: PDC_BTLB error %d", error);
#endif
	} else {
#ifdef BTLBDEBUG
		printf("btlb info: minsz=%d, maxsz=%d\n",
		    pdc_btlb.min_size, pdc_btlb.max_size);
		printf("btlb fixed: i=%d, d=%d, c=%d\n",
		    pdc_btlb.finfo.num_i,
		    pdc_btlb.finfo.num_d,
		    pdc_btlb.finfo.num_c);
		printf("btlb varbl: i=%d, d=%d, c=%d\n",
		    pdc_btlb.vinfo.num_i,
		    pdc_btlb.vinfo.num_d,
		    pdc_btlb.vinfo.num_c);
#endif /* BTLBDEBUG */
		/* purge TLBs and caches */
		if (pdc_call((iodcio_t)pdc, 0, PDC_BLOCK_TLB,
		    PDC_BTLB_PURGE_ALL) < 0)
			printf("WARNING: BTLB purge failed\n");

		cpu_features = pdc_btlb.finfo.num_c?
		    HPPA_FTRS_BTLBU : HPPA_FTRS_BTLBS;
	}

	ptlball();
	fcacheall();

	totalphysmem = PAGE0->imm_max_mem / NBPG;
	resvmem = ((vaddr_t)&kernel_text) / NBPG;

	/* calculate HPT size */
	/* for (hptsize = 256; hptsize < totalphysmem; hptsize *= 2); */
hptsize=256;	/* XXX one page for now */
	hptsize *= 16;	/* sizeof(hpt_entry) */

	if (pdc_call((iodcio_t)pdc, 0, PDC_TLB, PDC_TLB_INFO, &pdc_hwtlb) &&
	    !pdc_hwtlb.min_size && !pdc_hwtlb.max_size) {
		printf("WARNING: no HPT support, fine!\n");
		mtctl(hptsize - 1, CR_HPTMASK);
		hptsize = 0;
	} else {
		cpu_features |= HPPA_FTRS_HVT;

		if (hptsize > pdc_hwtlb.max_size)
			hptsize = pdc_hwtlb.max_size;
		else if (hptsize < pdc_hwtlb.min_size)
			hptsize = pdc_hwtlb.min_size;
		mtctl(hptsize - 1, CR_HPTMASK);
	}

	/*
	 * XXX fredette - much of this TLB trap handler setup should
	 * probably be moved here to hppa/hppa/hppa_machdep.c, seeing
	 * that there's related code already in hppa/hppa/trap.S.
	 */

	/*
	 * Deal w/ CPU now
	 */
	{
		const struct hppa_cpu_typed *p;

		for (p = cpu_types;
		     p->arch && p->features != cpu_features; p++);

		if (!p->arch)
			printf("WARNING: UNKNOWN CPU TYPE; GOOD LUCK (%x)\n",
			    cpu_features);
		else {
			/*
			 * Ptrs to various tlb handlers, to be filled
			 * based on cpu features.
			 * from locore.S
			 */
			extern u_int trap_ep_T_TLB_DIRTY[];
			extern u_int trap_ep_T_DTLBMISS[];
			extern u_int trap_ep_T_DTLBMISSNA[];
			extern u_int trap_ep_T_ITLBMISS[];
			extern u_int trap_ep_T_ITLBMISSNA[];

			cpu_type      = p->type;
			cpu_typename  = p->name;
			cpu_ibtlb_ins = p->ibtlbins;
			cpu_dbtlb_ins = p->dbtlbins;
			cpu_hpt_init  = p->hptinit;
			cpu_desidhash = p->desidhash;

#define	LDILDO(t,f) ((t)[0] = (f)[0], (t)[1] = (f)[1])
			LDILDO(trap_ep_T_TLB_DIRTY , p->tlbdh);
			LDILDO(trap_ep_T_DTLBMISS  , p->dtlbh);
			LDILDO(trap_ep_T_DTLBMISSNA, p->dtlbnah);
			LDILDO(trap_ep_T_ITLBMISS  , p->itlbh);
			LDILDO(trap_ep_T_ITLBMISSNA, p->itlbh);
#undef LDILDO
		}
	}

	/* we hope this won't fail */
	hppa_ex = extent_create("mem", 0x0, 0xffffffff, M_DEVBUF,
	    (caddr_t)mem_ex_storage, sizeof(mem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
	if (extent_alloc_region(hppa_ex, 0, (vaddr_t)PAGE0->imm_max_mem,
	    EX_NOWAIT))
		panic("cannot reserve main memory");

	vstart = hppa_round_page(start);
	vend = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Now allocate kernel dynamic variables
	 */

	physmem = totalphysmem;
	sz = (int)allocsys(NULL, NULL);
	memset((void *)vstart, 0, sz);
	if (allocsys((caddr_t)vstart, NULL) - (caddr_t)vstart != sz)
		panic("startup: table size inconsistency");
	vstart += sz;
	vstart = hppa_round_page(vstart);

	/* Calculate the OS_TOC handler checksum. */
	p = (u_int *) &os_toc;
	q = (&os_toc_end - 1);
	for(*q = 0; p < q; *q -= *(p++));

	/* Install the OS_TOC handler. */
	PAGE0->ivec_toc = os_toc;
	PAGE0->ivec_toclen = ((caddr_t) &os_toc_end) - ((caddr_t) &os_toc);

	pmap_bootstrap(&vstart, &vend);
	physmem = totalphysmem - btoc(vstart);

	/*
	 * BELOW THIS LINE REFERENCING PAGE0 AND OTHER LOW MEMORY 
	 * LOCATIONS, AND WRITING THE KERNEL TEXT IS PROHIBITED
	 * WITHOUT TAKING SPECIAL MEASURES.
	 */

	/* alloc msgbuf */
	if (!(msgbufaddr = (void *)pmap_steal_memory(MSGBUFSIZE, NULL, NULL)))
		panic("cannot allocate msgbuf");

	/* allocate the 24-bit DMA region. */
	v = NULL;
	dma24size = DMA24_SIZE_MAX;
	while (dma24size >= DMA24_SIZE_MIN) {
		v = pmap_steal_memory(dma24size, NULL, NULL);
		if (v != NULL)
			break;
	    dma24size -= DMA24_SIZE_STEP;
	}
	if (v == NULL) {
		printf("WARNING: could not allocate 24-bit DMA memory\n");
		dma24_ex = NULL;
	} else {
		dma24_ex = extent_create("dma24", v, v + dma24size, M_DEVBUF,
		    (caddr_t)dma24_ex_storage, sizeof(dma24_ex_storage),
		    EX_NOCOALESCE|EX_NOWAIT);
	}

	/* Turn on the HW TLB assist */
	if (hptsize) {
		u_int hpt;

		mfctl(CR_VTOP, hpt);
		if ((error = (cpu_hpt_init)(hpt, hptsize)) < 0) {
#ifdef DEBUG
			printf("WARNING: HPT init error %d\n", error);
#endif
		} else {
#ifdef PMAPDEBUG
			printf("HPT: %d entries @ 0x%x\n",
			    hptsize / sizeof(struct hpt_entry), hpt);
#endif
		}
	}

	/* locate coprocessors and SFUs */
	if ((error = pdc_call((iodcio_t)pdc, 0, PDC_COPROC, PDC_COPROC_DFLT,
	    &pdc_coproc)) < 0) {
		printf("WARNING: PDC_COPROC error %d\n", error);
		pdc_coproc.ccr_enable = 0;
	} else {
#ifdef DEBUG
		printf("pdc_coproc: %x, %x\n", pdc_coproc.ccr_enable,
		    pdc_coproc.ccr_present);
#endif
	}
	mtctl(pdc_coproc.ccr_enable & CCR_MASK, CR_CCR);
	
	/* Bootstrap any FPU. */
	hppa_fpu_bootstrap(pdc_coproc.ccr_enable);

	/* they say PDC_COPROC might turn fault light on */
	pdc_call((iodcio_t)pdc, PDC_CHASSIS, PDC_CHASSIS_DISP,
	    PDC_OSTAT(PDC_OSTAT_RUN) | 0xCEC0);

	/* Bootstrap interrupt masking and dispatching. */
	hp700_intr_bootstrap();

#ifdef DDB
	{
		extern int end[];

		ddb_init(1, end, (int*)esym);
	}
#endif

	/* We will shortly go virtual. */
	pagezero_mapped = 0;
}

void
cpu_startup()
{
	struct pdc_model pdc_model PDC_ALIGNMENT;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	int base, residual;
	int err, i;
	char pbuf[3][9];
#ifdef PMAPDEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/* Initialize the message buffer. */
	initmsgbuf(msgbufaddr, MSGBUFSIZE);

	/*
	 * i won't understand a friend of mine,
	 * who sat in a room full of artificial ice,
	 * fogging the air w/ humid cries --
	 *	WELCOME TO SUMMER!
	 */
	printf(version);

	/* identify system type */
	if ((err = pdc_call((iodcio_t)pdc, 0, PDC_MODEL, PDC_MODEL_INFO,
	    &pdc_model)) < 0) {
#ifdef DEBUG
		printf("WARNING: PDC_MODEL error %d\n", err);
#endif
	} else {
		const char *p, *q;
		i = pdc_model.hvers >> 4;
		p = hppa_mod_info(HPPA_TYPE_BOARD, i);
		switch (pdc_model.arch_rev) {
		default:
		case 0:
			q = "1.0";
#ifdef COMPAT_HPUX
			cpu_model_hpux = HPUX_SYSCONF_CPUPA10;
#endif
			break;
		case 4:
			q = "1.1";
#ifdef COMPAT_HPUX
			cpu_model_hpux = HPUX_SYSCONF_CPUPA11;
#endif
			break;
		case 8:
			q = "2.0";
#ifdef COMPAT_HPUX
			cpu_model_hpux = HPUX_SYSCONF_CPUPA20;
#endif
			break;
		}

		if (p)
			sprintf(cpu_model, "HP9000/%s PA-RISC %s", p, q);
		else
			sprintf(cpu_model, "HP9000/(UNKNOWN %x) PA-RISC %s",
				i, q);
		printf("%s\n", cpu_model);
	}

	format_bytes(pbuf[0], sizeof(pbuf[0]), ptoa(totalphysmem));
	format_bytes(pbuf[1], sizeof(pbuf[1]), ptoa(resvmem));
	format_bytes(pbuf[2], sizeof(pbuf[2]), ptoa(physmem));
	printf("real mem = %s (%s reserved for PROM, %s used by NetBSD)\n",
	    pbuf[0], pbuf[1], pbuf[2]);

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
	    NULL, UVM_UNKNOWN_OFFSET, 0, UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE,
	    UVM_INH_NONE, UVM_ADV_NORMAL, 0)) != 0)
		panic("cpu_startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = PAGE_SIZE * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			if ((pg = uvm_pagealloc(NULL, 0, NULL, 0)) == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
				VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, FALSE, NULL);

	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, FALSE, NULL);

#ifdef PMAPDEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf[0], sizeof(pbuf[0]), ptoa(uvmexp.free));
	format_bytes(pbuf[1], sizeof(pbuf[1]), bufpages * PAGE_SIZE);
	printf("avail mem = %s\n", pbuf[0]);
	printf("using %d buffers containing %s of memory\n",
	    nbuf, pbuf[1]);

	/*
	 * Allocate a virtual page (for use by /dev/mem)
	 * This page is handed to pmap_enter() therefore
	 * it has to be in the normal kernel VA range.
	 */
	vmmap = uvm_km_valloc_wait(kernel_map, NBPG);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

}

/*
 * compute cpu clock ratio such as:
 *	cpu_ticksnum / cpu_ticksdenom = t + delta
 *	delta -> 0
 */
void
delay_init(void)
{
	register u_int num, denom, delta, mdelta;

	mdelta = UINT_MAX;
	for (denom = 1; denom < 1000; denom++) {
		num = (PAGE0->mem_10msec * denom) / 10000;
		delta = num * 10000 / denom - PAGE0->mem_10msec;
		if (!delta) {
			cpu_ticksdenom = denom;
			cpu_ticksnum = num;
			break;
		} else if (delta < mdelta) {
			cpu_ticksdenom = denom;
			cpu_ticksnum = num;
		}
	}
}

void
delay(us)
	u_int us;
{
	register u_int start, end, n;

	mfctl(CR_ITMR, start);
	while (us) {
		n = min(1000, us);
		end = start + n * cpu_ticksnum / cpu_ticksdenom;

		/* N.B. Interval Timer may wrap around */
		if (end < start)
			do
				mfctl(CR_ITMR, start);
			while (start > end);

		do
			mfctl(CR_ITMR, start);
		while (start < end);

		us -= n;
		mfctl(CR_ITMR, start);
	}
}

static __inline void
fall(c_base, c_count, c_loop, c_stride, data)
	int c_base, c_count, c_loop, c_stride, data;
{
	register int loop;

	for (; c_count--; c_base += c_stride)
		for (loop = c_loop; loop--; )
			if (data)
				fdce(0, c_base);
			else
				fice(0, c_base);
}

void
fcacheall()
{
	/*
	 * Flush the instruction, then data cache.
	 */
	fall(pdc_cache.ic_base, pdc_cache.ic_count, pdc_cache.ic_loop,
	    pdc_cache.ic_stride, 0);
	sync_caches();
	fall(pdc_cache.dc_base, pdc_cache.dc_count, pdc_cache.dc_loop,
	    pdc_cache.dc_stride, 1);
	sync_caches();
}

void
ptlball()
{
	register pa_space_t sp;
	register int i, j, k;

	/* instruction TLB */
	sp = pdc_cache.it_sp_base;
	for (i = 0; i < pdc_cache.it_sp_count; i++) {
		register vaddr_t off = pdc_cache.it_off_base;
		for (j = 0; j < pdc_cache.it_off_count; j++) {
			for (k = 0; k < pdc_cache.it_loop; k++)
				pitlbe(sp, off);
			off += pdc_cache.it_off_stride;
		}
		sp += pdc_cache.it_sp_stride;
	}

	/* data TLB */
	sp = pdc_cache.dt_sp_base;
	for (i = 0; i < pdc_cache.dt_sp_count; i++) {
		register vaddr_t off = pdc_cache.dt_off_base;
		for (j = 0; j < pdc_cache.dt_off_count; j++) {
			for (k = 0; k < pdc_cache.dt_loop; k++)
				pdtlbe(sp, off);
			off += pdc_cache.dt_off_stride;
		}
		sp += pdc_cache.dt_sp_stride;
	}
}

int
desidhash_g()
{
	/* TODO call PDC to disable SID hashing in the cache index */

	return 0;
}

int
hpti_g(hpt, hptsize)
	vaddr_t hpt;
	vsize_t hptsize;
{
	return pdc_call((iodcio_t)pdc, 0, PDC_TLB, PDC_TLB_CONFIG,
	    &pdc_hwtlb, hpt, hptsize, PDC_TLB_CURRPDE);
}

int
pbtlb_g(i)
	int i;
{
	return -1;
}

int
ibtlb_g(i, sp, va, pa, sz, prot)
	int i;
	pa_space_t sp;
	vaddr_t va;
	paddr_t pa;
	vsize_t sz;
	u_int prot;
{
	int error;

	if ((error = pdc_call((iodcio_t)pdc, 0, PDC_BLOCK_TLB, PDC_BTLB_INSERT,
	    sp, va, pa, sz, prot, i)) < 0) {
#ifdef BTLBDEBUG
		printf("WARNING: BTLB insert failed (%d)\n", error);
#endif
	}
	return error;
}

/*
 * This inserts a recorded BTLB entry.
 */
static int _btlb_insert __P((struct btlb_entry *));
static int
_btlb_insert(struct btlb_entry *btlb_entry)
{
	int error;
#ifdef DEBUG
	const char *prot;

	/* Display the protection like a file protection. */
	switch (btlb_entry->btlb_entry_tlbprot & TLB_AR_MASK) {
	case TLB_AR_NA:		prot = "------"; break;
	case TLB_AR_KR:		prot = "r-----"; break;
	case TLB_AR_KRW:	prot = "rw----"; break;
	case TLB_AR_KRX:	prot = "r-x---"; break;
	case TLB_AR_KRWX:	prot = "rwx---"; break;
	case TLB_AR_UR:		prot = "r--r--"; break;
	case TLB_AR_URW:	prot = "rw-rw-"; break;
	case TLB_AR_URX:	prot = "r--r-x"; break;
	case TLB_AR_URWX:	prot = "rw-rwx"; break;
	default:		prot = "??????"; break;
	}

	printf("  [ BTLB entry %d: %s 0x%08x @ 0x%x:0x%08x len 0x%08x ]  ",
		btlb_entry->btlb_entry_num,
		prot,
		(u_int)btlb_entry->btlb_entry_pa_frame << PGSHIFT,
		btlb_entry->btlb_entry_va_space,
		(u_int)btlb_entry->btlb_entry_va_frame << PGSHIFT,
		(u_int)btlb_entry->btlb_entry_frames << PGSHIFT);
	/*
	 * Non-I/O space mappings are entered by the pmap,
	 * so we do print a newline to make things look better.
	 */
	if (btlb_entry->btlb_entry_pa_frame < (HPPA_IOSPACE >> PGSHIFT))
		printf("\n");
#endif
	error = (*cpu_dbtlb_ins)(btlb_entry->btlb_entry_num, 
				 btlb_entry->btlb_entry_va_space, 
				 btlb_entry->btlb_entry_va_frame, 
				 btlb_entry->btlb_entry_pa_frame, 
				 btlb_entry->btlb_entry_frames, 
				 btlb_entry->btlb_entry_tlbprot);
	return (error ? EINVAL : 0);
}

/*
 * This records and inserts a new BTLB entry.  It returns a cookie
 * that one day will be used with a btlb_purge function.
 * XXX - currently we only support machines with a combined BTLB.
 */
int
btlb_insert(pa_space_t space, vaddr_t va, paddr_t pa, 
		vsize_t *sizep, u_int tlbprot)
{
	struct btlb_entry *btlb_entry, *btlb_entry_end;
	vsize_t frames;
	int error;

	/* First, find a free BTLB entry record. */
	btlb_entry = btlb_entries;
	btlb_entry_end = btlb_entries + pdc_btlb.finfo.num_c;
	while (btlb_entry < btlb_entry_end && btlb_entry->btlb_entry_frames)
		btlb_entry++;
	if (btlb_entry == btlb_entry_end) {
#ifdef BTLBDEBUG
		printf("BTLB full\n");
#endif
		return -(ENOMEM);
	}

	/*
	 * Now calculate the number of pages (frames) that this
	 * BTLB entry will map.  It must be a power of two at
	 * least as big as the BTLB minimum size.
	 */
	frames = pdc_btlb.min_size << PGSHIFT;
	while (frames < *sizep)
		frames <<= 1;
	frames >>= PGSHIFT;
	if (frames > pdc_btlb.max_size) {
#ifdef BTLBDEBUG
		printf("btlb_insert: too big (%u < %u < %u)\n",
		    pdc_btlb.min_size, frames, pdc_btlb.max_size);
#endif
		return -(ENOMEM);
	}

	/* I/O space must be mapped uncached. */
	if (pa >= HPPA_IOBEGIN)
		tlbprot |= TLB_UNCACHEABLE;

	/*
 	 * Turn the physical and virtual addresses into frame numbers
 	 * and check their alignment.
	 */
	pa >>= PGSHIFT;
	va >>= PGSHIFT;
	if (pa & (frames - 1))
		printf("WARNING: BTLB address misaligned\n");

	/*
	 * Now fill this BTLB entry record and insert the entry.
	 */
	btlb_entry->btlb_entry_va_space = space;
	btlb_entry->btlb_entry_va_frame = va;
	btlb_entry->btlb_entry_pa_frame = pa;
	btlb_entry->btlb_entry_frames = frames;
	btlb_entry->btlb_entry_tlbprot = tlbprot;
	btlb_entry->btlb_entry_num = (btlb_entry - btlb_entries);
	error = _btlb_insert(btlb_entry);
	if (error)
		return -(error);

	/* Success. */
	*sizep = (frames << PGSHIFT);
	return (btlb_entry - btlb_entries);
}

/*
 * This reloads the BTLB in the event that it becomes invalidated.
 */
int
hppa_btlb_reload(void)
{
	struct btlb_entry *btlb_entry, *btlb_entry_end;
	int error;

	/* Insert all recorded BTLB entries. */
	btlb_entry = btlb_entries;
	btlb_entry_end = btlb_entries + 
		(sizeof(btlb_entries) / sizeof(btlb_entries[0]));
	error = 0;
	while (error == 0 && btlb_entry < btlb_entry_end) {
		if (btlb_entry->btlb_entry_frames)
			error = _btlb_insert(btlb_entry);
		btlb_entry++;
	}
#ifdef DEBUG
	printf("\n");
#endif
	return (error);
}

/*
 * This maps page zero if it isn't already mapped, and 
 * returns a cookie for hp700_pagezero_unmap.
 */
int
hp700_pagezero_map(void)
{
	int was_mapped_before;
	int s;

	was_mapped_before = pagezero_mapped;
	if (!was_mapped_before) {
		s = splhigh();
		pmap_kenter_pa(0, 0, VM_PROT_ALL);
		pagezero_mapped = 1;
		splx(s);
	}
	return (was_mapped_before);
}

/*
 * This unmaps mape zero, given a cookie previously returned 
 * by hp700_pagezero_map.
 */
void
hp700_pagezero_unmap(int was_mapped_before)
{
	int s;

	if (!was_mapped_before) {
		s = splhigh();
		pmap_kremove(0, NBPG);
		pagezero_mapped = 0;
		splx(s);
	}
}

int waittime = -1;

__dead void
cpu_reboot(howto, user_boot_string)
	int howto;
	char *user_boot_string;
{

	/* If the system is cold, just give up and halt. */
	if (cold)
		goto haltsys;

	boothowto = howto | (boothowto & RB_HALT);

	if (!(howto & RB_NOSYNC) && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
#if 0
		if ((howto & RB_TIMEBAD) == 0)
			resettodr();
		else
#endif
			printf("WARNING: not updating battery clock\n");
	}

	/* XXX probably save howto into stable storage */

	/* Disable interrupts. */
	splhigh();

	/* Make a crash dump. */
	if (howto & RB_DUMP)
		dumpsys();

	/* Run any shutdown hooks. */
	doshutdownhooks();

	if (howto & RB_HALT) {
haltsys:
		printf("System halted!\n");
		__asm __volatile("stwas %0, 0(%1)"
		    :: "r" (CMD_STOP), "r" (LBCAST_ADDR + iomod_command));
	} else {
		printf("rebooting...");
		DELAY(1000000);
		__asm __volatile("stwas %0, 0(%1)"
		    :: "r" (CMD_RESET), "r" (LBCAST_ADDR + iomod_command));
	}

	for(;;); /* loop while bus reset is comming up */
	/* NOTREACHED */
}

u_long	dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
int
cpu_dumpsize()
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	if (roundup(size, dbtob(1)) != dbtob(1))
		return -1;

	return 1;
}

/*
 * This handles a machine check.  This can be either an HPMC, 
 * an LPMC, or a TOC.  The check type is passed in as a trap
 * type, one of T_HPMC, T_LPMC, or T_INTERRUPT (for TOC).
 */
static char pim_data_buffer[4096];	/* XXX assumed to be big enough */
static char in_check = 0;
void
hppa_machine_check(int check_type)
{
	int pdc_pim_type;
	struct hp700_pim_hpmc *hpmc;
	struct hp700_pim_lpmc *lpmc;
	struct hp700_pim_toc *toc;
	struct hp700_pim_regs *regs;
	struct hp700_pim_checks *checks;
	u_int *regarray;
	int reg_i, reg_j, reg_k;
	char bitmask_buffer[64];
	const char *name;
	int error;
#define	PIM_WORD(name, word, bits)			\
do {							\
	bitmask_snprintf(word, bits, bitmask_buffer,	\
		sizeof(bitmask_buffer));		\
	printf("%s %s", name, bitmask_buffer);		\
} while (/* CONSTCOND */ 0)

	/* Do an fcacheall(). */
	fcacheall();

	/* Dispatch on the check type. */
	regs = NULL;
	checks = NULL;
	switch (check_type) {
	case T_HPMC:
		name = "HPMC";
		pdc_pim_type = PDC_PIM_HPMC;
		hpmc = (struct hp700_pim_hpmc *) pim_data_buffer;
		regs = &hpmc->pim_hpmc_regs;
		checks = &hpmc->pim_hpmc_checks;
		break;
	case T_LPMC:
		name = "LPMC";
		pdc_pim_type = PDC_PIM_LPMC;
		lpmc = (struct hp700_pim_lpmc *) pim_data_buffer;
		checks = &lpmc->pim_lpmc_checks;
		break;
	case T_INTERRUPT:
		name = "TOC";
		pdc_pim_type = PDC_PIM_TOC;
		toc = (struct hp700_pim_toc *) pim_data_buffer;
		regs = &toc->pim_toc_regs;
		break;
	default:
		panic("unknown machine check type");
		/* NOTREACHED */
	}
	printf("\nmachine check: %s", name);
	error = pdc_call((iodcio_t)pdc, 0, PDC_PIM, pdc_pim_type, 
		&pdc_pim, pim_data_buffer, sizeof(pim_data_buffer));
	if (error < 0)
		printf(" - WARNING: could not transfer PIM info (%d)", error);

	/* If we have register arrays, display them. */
	if (regs != NULL) {
		for (reg_i = 0; reg_i < 3; reg_i++) {
			if (reg_i == 0) {
				name = "General";
				regarray = &regs->pim_regs_r0;
				reg_j = 32;
			} else if (reg_i == 1) {
				name = "Control";
				regarray = &regs->pim_regs_cr0;
				reg_j = 32;
			} else {
				name = "Space";
				regarray = &regs->pim_regs_sr0;
				reg_j = 8;
			}
			printf("\n\n\t%s Registers:", name);
			for (reg_k = 0; reg_k < reg_j; reg_k++)
				printf("%s0x%08x",
					(reg_k & 3) ? " " : "\n\t",
					regarray[reg_k]);
		}

		/* Print out some interesting registers. */
		printf("\n\n\tIIA 0x%x:0x%08x 0x%x:0x%08x", 
			regs->pim_regs_cr17, regs->pim_regs_cr18,
			regs->pim_regs_iisq_tail, regs->pim_regs_iioq_tail);
		PIM_WORD("\n\tIPSW", regs->pim_regs_cr22, PSW_BITS);
		printf("\n\tSP 0x%x:0x%08x FP 0x%x:0x%08x",
			regs->pim_regs_sr0, regs->pim_regs_r30,
			regs->pim_regs_sr0, regs->pim_regs_r3);
	}

	/* If we have check words, display them. */
	if (checks != NULL) {
		PIM_WORD("\n\n\tCheck Type", checks->pim_check_type, 
			PIM_CHECK_BITS);
		PIM_WORD("\n\tCPU State", checks->pim_check_cpu_state, 
			PIM_CPU_BITS PIM_CPU_HPMC_BITS);
		PIM_WORD("\n\tCache Check", checks->pim_check_cache, 
			PIM_CACHE_BITS);
		PIM_WORD("\n\tTLB Check", checks->pim_check_tlb, 
			PIM_TLB_BITS);
		PIM_WORD("\n\tBus Check", checks->pim_check_bus, 
			PIM_BUS_BITS);
		PIM_WORD("\n\tAssist Check", checks->pim_check_assist, 
			PIM_ASSIST_BITS);
		printf("\tAssist State %u", checks->pim_check_assist_state);
		printf("\n\tSystem Responder 0x%08x", 
			checks->pim_check_responder);
		printf("\n\tSystem Requestor 0x%08x", 
			checks->pim_check_requestor);
		printf("\n\tPath Info 0x%08x", 
			checks->pim_check_path_info);
	}
	printf("\n");

	/* If this is our first check, panic. */
	if (in_check == 0) {
		in_check = 1;
		DELAY(250000);
		panic("machine check");
	}
	
	/* Reboot the machine. */
	printf("Rebooting...\n");
	cpu_die();
}

int
cpu_dump()
{
	long buf[dbtob(1) / sizeof (long)];
	kcore_seg_t	*segp;
	cpu_kcore_hdr_t	*cpuhdrp;

	segp = (kcore_seg_t *)buf;
	cpuhdrp = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(*segp)) / sizeof (long)];

	/*
	 * Generate a segment header.
	 */
	CORE_SETMAGIC(*segp, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	segp->c_size = dbtob(1) - ALIGN(sizeof(*segp));

	/*
	 * Add the machine-dependent header info
	 */
	/* nothing for now */

	return (bdevsw[major(dumpdev)].d_dump)
	    (dumpdev, dumplo, (caddr_t)buf, dbtob(1));
}

/*
 * Dump the kernel's image to the swap partition.
 */
#define	BYTES_PER_DUMP	NBPG

void
dumpsys()
{
	int psize, bytes, i, n;
	register caddr_t maddr;
	register daddr_t blkno;
	register int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	register int error;

	if (dumpdev == NODEV)
		return;

	/* Save registers
	savectx(&dumppcb); */

	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0) {
		printf("\ndump to dev %x not possible\n", dumpdev);
		return;
	}
	printf("\ndumping to dev %x, offset %ld\n", dumpdev, dumplo);

	psize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	if (!(error = cpu_dump())) {

		bytes = ctob(physmem);
		maddr = NULL;
		blkno = dumplo + cpu_dumpsize();
		dump = bdevsw[major(dumpdev)].d_dump;
		/* TODO block map the whole physical memory */
		for (i = 0; i < bytes; i += n) {
		
			/* Print out how many MBs we are to go. */
			n = bytes - i;
			if (n && (n % (1024*1024)) == 0)
				printf("%d ", n / (1024 * 1024));

			/* Limit size for next transfer. */

			if (n > BYTES_PER_DUMP)
				n = BYTES_PER_DUMP;

			if ((error = (*dump)(dumpdev, blkno, maddr, n)))
				break;
			maddr += n;
			blkno += btodb(n);
		}
	}

	switch (error) {
	case ENXIO:	printf("device bad\n");			break;
	case EFAULT:	printf("device not ready\n");		break;
	case EINVAL:	printf("area improper\n");		break;
	case EIO:	printf("i/o error\n");			break;
	case EINTR:	printf("aborted from console\n");	break;
	case 0:		printf("succeeded\n");			break;
	default:	printf("error %d\n", error);		break;
	}
}

/* bcopy(), error on fault */
int
kcopy(from, to, size)
	const void *from;
	void *to;
	size_t size;
{
	register u_int oldh = curproc->p_addr->u_pcb.pcb_onfault;

	curproc->p_addr->u_pcb.pcb_onfault = (u_int)&copy_on_fault;
	bcopy(from, to, size);
	curproc->p_addr->u_pcb.pcb_onfault = oldh;

	return 0;
}

/*
 * Set registers on exec.
 */
void
setregs(p, pack, stack)
	register struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	register struct trapframe *tf = p->p_md.md_regs;
	/* register struct pcb *pcb = &p->p_addr->u_pcb; */
#ifdef PMAPDEBUG
	extern int pmapdebug;
	pmapdebug = 0x180d;
	pmapdebug = -1;
	printf("setregs(%p, %p, %x), ep=%x, cr30=%x\n",
	    p, pack, (u_int)stack, (u_int)pack->ep_entry, tf->tf_cr30);
#endif

	tf->tf_iioq_tail = 4 +
	    (tf->tf_iioq_head = pack->ep_entry | HPPA_PC_PRIV_USER);
	tf->tf_rp = 0;
	tf->tf_arg0 = (u_long)p->p_psstr;
	tf->tf_arg1 = tf->tf_arg2 = 0; /* XXX dynload stuff */

	/* setup terminal stack frame */
	stack = (u_long)STACK_ALIGN(stack, 63);
	stack += HPPA_FRAME_SIZE;
	suword((caddr_t)(stack + HPPA_FRAME_PSP), 0);
	tf->tf_sp = stack;
}

/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	dev_t consdev;
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);	/* overloaded */
	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}


/*
 * consinit:
 * initialize the system console.
 */
void
consinit()
{
	static int initted = 0;

	if (!initted) {
		initted++;
		cninit();
	}
}
