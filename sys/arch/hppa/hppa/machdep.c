/*	$NetBSD: machdep.c,v 1.10.4.1 2022/05/14 11:38:31 martin Exp $	*/

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
 * Copyright (c) 1999-2003 Michael Shalayeff
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.10.4.1 2022/05/14 11:38:31 martin Exp $");

#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"
#include "opt_useleds.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/cpu.h>
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
#include <sys/exec.h>
#include <sys/exec_aout.h>		/* for MID_* */
#include <sys/sysctl.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/module.h>
#include <sys/extent.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/mutex.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_page.h>
#include <uvm/uvm.h>

#include <dev/cons.h>
#include <dev/mm.h>

#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/cpufunc.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/kcore.h>
#include <machine/pcb.h>

#ifdef	KGDB
#include "com.h"
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include <hppa/hppa/machdep.h>
#include <hppa/hppa/pim.h>
#include <hppa/dev/cpudevs.h>

#ifdef PMAPDEBUG
#include <hppa/hppa/hpt.h>
#endif

#include "ksyms.h"
#include "lcd.h"

#ifdef MACHDEPDEBUG

#define	DPRINTF(s)	do {		\
	if (machdepdebug)		\
		printf s;		\
} while(0)

#define	DPRINTFN(l,s)	do {		\
	if (machdepdebug >= (1))	\
		printf s;		\
} while(0)

int machdepdebug = 1;
#else
#define	DPRINTF(s)	/* */
#define	DPRINTFN(l,s)	/* */
#endif

/*
 * Different kinds of flags used throughout the kernel.
 */
void *msgbufaddr;

/* The primary (aka monarch) cpu HPA */
hppa_hpa_t hppa_mcpuhpa;

/*
 * cache configuration, for most machines is the same
 * numbers, so it makes sense to do defines w/ numbers depending
 * on configured CPU types in the kernel
 */
int icache_stride, icache_line_mask;
int dcache_stride, dcache_line_mask;

/*
 * things to not kill
 */
volatile uint8_t *machine_ledaddr;
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
struct pdc_cache pdc_cache;
struct pdc_btlb pdc_btlb;
struct pdc_model pdc_model;

int usebtlb;

/*
 * The BTLB slots.
 */
static struct btlb_slot {

	/* The number associated with this slot. */
	int btlb_slot_number;

	/* The flags associated with this slot. */
	int btlb_slot_flags;
#define	BTLB_SLOT_IBTLB			(1 << 0)
#define	BTLB_SLOT_DBTLB			(1 << 1)
#define	BTLB_SLOT_CBTLB			(BTLB_SLOT_IBTLB | BTLB_SLOT_DBTLB)
#define	BTLB_SLOT_VARIABLE_RANGE	(1 << 2)

	/*
	 * The mapping information.  A mapping is free
	 * if its btlb_slot_frames member is zero.
	 */
	pa_space_t btlb_slot_va_space;
	vaddr_t	btlb_slot_va_frame;
	paddr_t btlb_slot_pa_frame;
	vsize_t btlb_slot_frames;
	u_int btlb_slot_tlbprot;
} *btlb_slots;
int	btlb_slots_count;

/* w/ a little deviation should be the same for all installed cpus */
u_int	cpu_ticksnum, cpu_ticksdenom, cpu_hzticks;

/* exported info */
char	machine[] = MACHINE;
const struct hppa_cpu_info *hppa_cpu_info;
enum hppa_cpu_type cpu_type;
int	cpu_modelno;
int	cpu_revision;

#if NLCD > 0
bool	lcd_blink_p;
#endif

/*
 * exported methods for cpus
 */
int (*cpu_desidhash)(void);
int (*cpu_hpt_init)(vaddr_t, vsize_t);
int (*cpu_ibtlb_ins)(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
int (*cpu_dbtlb_ins)(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);

dev_t	bootdev;
int	totalphysmem;		/* # pages in system */
int	availphysmem;		/* # pages available to kernel */
int	esym;
paddr_t	avail_end;

/*
 * Our copy of the bootinfo struct passed to us by the boot loader.
 */
struct bootinfo bootinfo;

/*
 * XXX note that 0x12000 is the old kernel text start
 * address.  Memory below this is assumed to belong
 * to the firmware.  This value is converted into pages
 * by hppa_init and used as pages in pmap_bootstrap().
 */
int	resvmem = 0x12000;
int	resvphysmem;

/*
 * BTLB parameters, broken out for the MI hppa code.
 */
u_int hppa_btlb_size_min, hppa_btlb_size_max;

/*
 * Things for MI glue to stick on.
 */
struct extent *hppa_io_extent;
static long hppa_io_extent_store[EXTENT_FIXED_STORAGE_SIZE(64) / sizeof(long)];

struct pool hppa_fppl;
struct fpreg lwp0_fpregs;

/* Our exported CPU info */
struct cpu_info cpus[HPPA_MAXCPUS] = {
#ifdef MULTIPROCESSOR
	{
		.ci_curlwp = &lwp0,
	},
#endif
};

struct vm_map *phys_map = NULL;

void delay_init(void);
static inline void fall(int, int, int, int, int);
void dumpsys(void);
void cpuid(void);
enum hppa_cpu_type cpu_model_cpuid(int);
#if NLCD > 0
void blink_lcd_timeout(void *);
#endif

/*
 * wide used hardware params
 */
struct pdc_hwtlb pdc_hwtlb;
struct pdc_coproc pdc_coproc;
struct pdc_coherence pdc_coherence;
struct pdc_spidb pdc_spidbits;
struct pdc_pim pdc_pim;
struct pdc_model pdc_model;

/*
 * Debugger info.
 */
int hppa_kgdb_attached;

/*
 * Whatever CPU types we support
 */
extern const u_int itlb_x[], itlbna_x[], dtlb_x[], dtlbna_x[], tlbd_x[];
extern const u_int itlb_s[], itlbna_s[], dtlb_s[], dtlbna_s[], tlbd_s[];
extern const u_int itlb_t[], itlbna_t[], dtlb_t[], dtlbna_t[], tlbd_t[];
extern const u_int itlb_l[], itlbna_l[], dtlb_l[], dtlbna_l[], tlbd_l[];
extern const u_int itlb_u[], itlbna_u[], dtlb_u[], dtlbna_u[], tlbd_u[];

int iibtlb_s(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
int idbtlb_s(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
int ibtlb_t(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
int ibtlb_l(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
int ibtlb_u(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
int ibtlb_g(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
int pbtlb_g(int);
int pbtlb_u(int);
int hpti_l(vaddr_t, vsize_t);
int hpti_u(vaddr_t, vsize_t);
int hpti_g(vaddr_t, vsize_t);
int desidhash_x(void);
int desidhash_s(void);
int desidhash_t(void);
int desidhash_l(void);
int desidhash_u(void);

const struct hppa_cpu_info cpu_types[] = {
#ifdef HP7000_CPU
	{ "PA7000", NULL, "PCX",
	  hpcx,  0,
	  0, "1.0",
	  desidhash_x, itlb_x, dtlb_x, itlbna_x, dtlbna_x, tlbd_x,
	  ibtlb_g, NULL, pbtlb_g, NULL }, /* XXXNH check */
#endif
#ifdef HP7000_CPU
	{ "PA7000", NULL, "PCXS",
	  hpcxs,  0,
	  0, "1.1a",
	  desidhash_s, itlb_s, dtlb_s, itlbna_s, dtlbna_s, tlbd_s,
	  ibtlb_g, NULL, pbtlb_g, NULL },
#endif
#ifdef HP7100_CPU
	{ "PA7100", "T-Bird", "PCXT",
	  hpcxt, 0,
	  HPPA_FTRS_BTLBU, "1.1b",
	  desidhash_t, itlb_t, dtlb_t, itlbna_t, dtlbna_t, tlbd_t,
	  ibtlb_g, NULL, pbtlb_g, NULL },
#endif
#ifdef HP7100LC_CPU
	{ "PA7100LC", "Hummingbird", "PCXL",
	  hpcxl, HPPA_CPU_PCXL,
	  HPPA_FTRS_TLBU | HPPA_FTRS_BTLBU | HPPA_FTRS_HVT, "1.1c",
	  desidhash_l, itlb_l, dtlb_l, itlbna_l, dtlbna_l, tlbd_l,
	  ibtlb_g, NULL, pbtlb_g, hpti_g },
#endif
#ifdef HP7200_CPU
	{ "PA7200", "T-Bird", "PCXT'",
	  hpcxtp, HPPA_CPU_PCXT2,
	  HPPA_FTRS_BTLBU, "1.1d",
	  desidhash_t, itlb_t, dtlb_t, itlbna_t, dtlbna_t, tlbd_t,
	  ibtlb_g, NULL, pbtlb_g, NULL },
#endif
#ifdef HP7300LC_CPU
	{ "PA7300LC", "Velociraptor", "PCXL2",
	  hpcxl2, HPPA_CPU_PCXL2,
	  HPPA_FTRS_TLBU | HPPA_FTRS_BTLBU | HPPA_FTRS_HVT, "1.1e",
	  NULL, itlb_l, dtlb_l, itlbna_l, dtlbna_l, tlbd_l,
	  ibtlb_g, NULL, pbtlb_g, hpti_g },
#endif
#ifdef HP8000_CPU
	{ "PA8000", "Onyx", "PCXU",
	  hpcxu, HPPA_CPU_PCXU,
	  HPPA_FTRS_W32B, "2.0",
	  desidhash_u, itlb_u, dtlb_u, itlbna_u, dtlbna_u, tlbd_u,
 	  ibtlb_u, NULL, pbtlb_u, NULL },
#endif
#ifdef HP8200_CPU
	{ "PA8200", "Vulcan", "PCXU+",
	  hpcxup, HPPA_CPU_PCXUP,
	  HPPA_FTRS_W32B, "2.0",
	  desidhash_u, itlb_u, dtlb_u, itlbna_u, dtlbna_u, tlbd_u,
 	  ibtlb_u, NULL, pbtlb_u, NULL },
#endif
#ifdef HP8500_CPU
	{ "PA8500", "Barra'Cuda", "PCXW",
	  hpcxw, HPPA_CPU_PCXW,
	  HPPA_FTRS_W32B, "2.0",
	  desidhash_u, itlb_u, dtlb_u, itlbna_u, dtlbna_u, tlbd_u,
 	  ibtlb_u, NULL, pbtlb_u, NULL },
#endif
#ifdef HP8600_CPU
	{ "PA8600", "Landshark", "PCXW+",
	  hpcxwp, HPPA_CPU_PCXWP,
	  HPPA_FTRS_W32B, "2.0",
	  desidhash_u, itlb_u, dtlb_u, itlbna_u, dtlbna_u, tlbd_u,
 	  ibtlb_u, NULL, pbtlb_u, NULL },
#endif
#ifdef HP8700_CPU
	{ "PA8700", "Piranha", "PCXW2",
	  hpcxw2, HPPA_CPU_PCXW2,
	  HPPA_FTRS_W32B, "2.0",
	  desidhash_u, itlb_u, dtlb_u, itlbna_u, dtlbna_u, tlbd_u,
 	  ibtlb_u, NULL, pbtlb_u, NULL },
#endif
#ifdef HP8800_CPU
	{ "PA8800", "Mako", "Make",
	  mako, HPPA_CPU_PCXW2,
	  HPPA_FTRS_W32B, "2.0",
	  desidhash_u, itlb_u, dtlb_u, itlbna_u, dtlbna_u, tlbd_u,
 	  ibtlb_u, NULL, pbtlb_u, NULL },
#endif
#ifdef HP8900_CPU
	{ "PA8900", "Shortfin", "Shortfin",
	  mako, HPPA_CPU_PCXW2,
	  HPPA_FTRS_W32B, "2.0",
	  desidhash_u, itlb_u, dtlb_u, itlbna_u, dtlbna_u, tlbd_u,
 	  ibtlb_u, NULL, pbtlb_u, NULL },
#endif
};

void
hppa_init(paddr_t start, void *bi)
{
	vaddr_t vstart;
	vaddr_t v;
	int error;
	u_int *p, *q;
	struct btlb_slot *btlb_slot;
	int btlb_slot_i;
	struct btinfo_symtab *bi_sym;
	struct pcb *pcb0;
	struct cpu_info *ci;

#ifdef KGDB
	boothowto |= RB_KDB;	/* go to kgdb early if compiled in. */
#endif
	/* Setup curlwp/curcpu early for LOCKDEBUG and spl* */
#ifdef MULTIPROCESSOR
	mtctl(&cpus[0], CR_CURCPU);
#else
	mtctl(&lwp0, CR_CURLWP);
#endif
	lwp0.l_cpu = &cpus[0];

	/* curcpu() is now valid */
	ci = curcpu();

	ci->ci_psw =
		PSW_Q |         /* Interrupt State Collection Enable */
		PSW_P |         /* Protection Identifier Validation Enable */
		PSW_C |         /* Instruction Address Translation Enable */
		PSW_D;          /* Data Address Translation Enable */

	/* Copy bootinfo */
	if (bi != NULL)
		memcpy(&bootinfo, bi, sizeof(struct bootinfo));

	/* init PDC iface, so we can call em easy */
	pdc_init();

	cpu_hzticks = (PAGE0->mem_10msec * 100) / hz;

	/* calculate CPU clock ratio */
	delay_init();

	/* fetch the monarch/"default" cpu hpa */
	error = pdcproc_hpa_processor(&hppa_mcpuhpa);
	if (error < 0)
		panic("%s: PDC_HPA failed", __func__);

	/* cache parameters */
	error = pdcproc_cache(&pdc_cache);
	if (error < 0) {
		DPRINTF(("WARNING: PDC_CACHE error %d\n", error));
	}

	dcache_line_mask = pdc_cache.dc_conf.cc_line * 16 - 1;
	dcache_stride = pdc_cache.dc_stride;
	icache_line_mask = pdc_cache.ic_conf.cc_line * 16 - 1;
	icache_stride = pdc_cache.ic_stride;

	error = pdcproc_cache_spidbits(&pdc_spidbits);
	DPRINTF(("SPID bits: 0x%x, error = %d\n", pdc_spidbits.spidbits, error));

	/* Calculate the OS_HPMC handler checksums. */
	p = &os_hpmc;
	if (pdcproc_instr(p))
		*p = 0x08000240;
	p[7] = ((char *) &os_hpmc_cont_end) - ((char *) &os_hpmc_cont);
	p[6] = (u_int) &os_hpmc_cont;
	p[5] = -(p[0] + p[1] + p[2] + p[3] + p[4] + p[6] + p[7]);
	p = &os_hpmc_cont;
	q = (&os_hpmc_cont_end - 1);
	for(*q = 0; p < q; *q -= *(p++));

	/* Calculate the OS_TOC handler checksum. */
	p = (u_int *) &os_toc;
	q = (&os_toc_end - 1);
	for (*q = 0; p < q; *q -= *(p++));

	/* Install the OS_TOC handler. */
	PAGE0->ivec_toc = os_toc;
	PAGE0->ivec_toclen = ((char *) &os_toc_end) - ((char *) &os_toc);

	cpuid();
	ptlball();
	fcacheall();

	avail_end = trunc_page(PAGE0->imm_max_mem);
	totalphysmem = atop(avail_end);
	if (avail_end > SYSCALLGATE)
		avail_end = SYSCALLGATE;
	physmem = atop(avail_end);
	resvmem = atop(resvmem);	/* XXXNH */

	/* we hope this won't fail */
	hppa_io_extent = extent_create("io",
	    HPPA_IOSPACE, 0xffffffff,
	    (void *)hppa_io_extent_store, sizeof(hppa_io_extent_store),
	    EX_NOCOALESCE|EX_NOWAIT);

	vstart = round_page(start);

	/*
	 * Now allocate kernel dynamic variables
	 */

	/* Allocate the msgbuf. */
	msgbufaddr = (void *) vstart;
	vstart += MSGBUFSIZE;
	vstart = round_page(vstart);

	if (usebtlb) {
		/* Allocate and initialize the BTLB slots array. */
		btlb_slots = (struct btlb_slot *) ALIGN(vstart);
		btlb_slot = btlb_slots;
#define BTLB_SLOTS(count, flags)					\
do {									\
	for (btlb_slot_i = 0;						\
	     btlb_slot_i < pdc_btlb.count;				\
	     btlb_slot_i++) {						\
		btlb_slot->btlb_slot_number = (btlb_slot - btlb_slots);	\
		btlb_slot->btlb_slot_flags = flags;			\
		btlb_slot->btlb_slot_frames = 0;			\
		btlb_slot++;						\
	}								\
} while (/* CONSTCOND */ 0)

		BTLB_SLOTS(finfo.num_i, BTLB_SLOT_IBTLB);
		BTLB_SLOTS(finfo.num_d, BTLB_SLOT_DBTLB);
		BTLB_SLOTS(finfo.num_c, BTLB_SLOT_CBTLB);
		BTLB_SLOTS(vinfo.num_i, BTLB_SLOT_IBTLB | BTLB_SLOT_VARIABLE_RANGE);
		BTLB_SLOTS(vinfo.num_d, BTLB_SLOT_DBTLB | BTLB_SLOT_VARIABLE_RANGE);
		BTLB_SLOTS(vinfo.num_c, BTLB_SLOT_CBTLB | BTLB_SLOT_VARIABLE_RANGE);
#undef BTLB_SLOTS

		btlb_slots_count = (btlb_slot - btlb_slots);
		vstart = round_page((vaddr_t) btlb_slot);
	}

	v = vstart;

	/* sets resvphysmem */
	pmap_bootstrap(v);

	/*
	 * BELOW THIS LINE REFERENCING PAGE0 AND OTHER LOW MEMORY
	 * LOCATIONS, AND WRITING THE KERNEL TEXT ARE PROHIBITED
	 * WITHOUT TAKING SPECIAL MEASURES.
	 */

	DPRINTF(("%s: PDC_CHASSIS\n", __func__));

	/* they say PDC_COPROC might turn fault light on */
	pdcproc_chassis_display(PDC_OSTAT(PDC_OSTAT_RUN) | 0xCEC0);

	DPRINTF(("%s: intr bootstrap\n", __func__));
	/* Bootstrap interrupt masking and dispatching. */
	hppa_intr_initialise(ci);

	/*
	 * Initialize any debugger.
	 */
#ifdef KGDB
	/*
	 * XXX note that we're not virtual yet, yet these
	 * KGDB attach functions will be using bus_space(9)
	 * to map and manipulate their devices.  This only
	 * works because, currently, the mainbus.c bus_space
	 * implementation directly-maps things in I/O space.
	 */
	hppa_kgdb_attached = false;
#if NCOM > 0
	if (!strcmp(KGDB_DEVNAME, "com")) {
		int com_gsc_kgdb_attach(void);
		if (com_gsc_kgdb_attach() == 0)
			hppa_kgdb_attached = true;
	}
#endif /* NCOM > 0 */
#endif /* KGDB */

#if NKSYMS || defined(DDB) || defined(MODULAR)
	if ((bi_sym = lookup_bootinfo(BTINFO_SYMTAB)) != NULL)
		ksyms_addsyms_elf(bi_sym->nsym, (int *)bi_sym->ssym,
		    (int *)bi_sym->esym);
	else {
		extern int end;

		ksyms_addsyms_elf(esym - (int)&end, &end, (int*)esym);
	}
#endif

	/* We will shortly go virtual. */
	pagezero_mapped = 0;
	fcacheall();

	pcb0 = lwp_getpcb(&lwp0);
	pcb0->pcb_fpregs = &lwp0_fpregs;
	memset(&lwp0_fpregs, 0, sizeof(struct fpreg));

	pool_init(&hppa_fppl, sizeof(struct fpreg), 16, 0, 0, "fppl", NULL,
	    IPL_NONE);
}

void
cpuid(void)
{
	/*
	 * XXX fredette - much of this TLB trap handler setup should
	 * probably be moved here to hppa/hppa/hppa_machdep.c, seeing
	 * that there's related code already in hppa/hppa/trap.S.
	 */

	/*
	 * Ptrs to various tlb handlers, to be filled
	 * based on CPU features.
	 * from locore.S
	 */
	extern u_int trap_ep_T_TLB_DIRTY[];
	extern u_int trap_ep_T_DTLBMISS[];
	extern u_int trap_ep_T_DTLBMISSNA[];
	extern u_int trap_ep_T_ITLBMISS[];
	extern u_int trap_ep_T_ITLBMISSNA[];

	struct pdc_cpuid pdc_cpuid;
	const struct hppa_cpu_info *p = NULL;
	const char *model;
	u_int cpu_version, cpu_features;
	int error, i;

	/* may the scientific guessing begin */
	cpu_type = hpc_unknown;
	cpu_features = 0;
	cpu_version = 0;

	/* identify system type */
	error = pdcproc_model_info(&pdc_model);
	if (error < 0) {
		DPRINTF(("WARNING: PDC_MODEL_INFO error %d\n", error));

		pdc_model.hwmodel = 0;
		pdc_model.hv = 0;
	} else {
		DPRINTF(("pdc_model.hwmodel/hv %x/%x\n", pdc_model.hwmodel,
		    pdc_model.hv));
	}
	cpu_modelno = pdc_model.hwmodel;
	model = hppa_mod_info(HPPA_TYPE_BOARD, cpu_modelno);

	DPRINTF(("%s: model %s\n", __func__, model));

	pdc_settype(cpu_modelno);

	memset(&pdc_cpuid, 0, sizeof(pdc_cpuid));
	error = pdcproc_model_cpuid(&pdc_cpuid);
	if (error < 0) {
		DPRINTF(("WARNING: PDC_MODEL_CPUID error %d. "
		    "Using cpu_modelno (%#x) based cpu_type.\n", error, cpu_modelno));

		cpu_type = cpu_model_cpuid(cpu_modelno);
		if (cpu_type == hpc_unknown) {
			printf("WARNING: Unknown cpu_type for cpu_modelno %x\n",
			   cpu_modelno);
		}
	} else {
		DPRINTF(("%s: cpuid.version  = %x\n", __func__,
		    pdc_cpuid.version));
		DPRINTF(("%s: cpuid.revision = %x\n", __func__,
		    pdc_cpuid.revision));

		cpu_version = pdc_cpuid.version;

		/* XXXNH why? */
		/* patch for old 8200 */
		if (pdc_cpuid.version == HPPA_CPU_PCXU &&
		    pdc_cpuid.revision > 0x0d)
			cpu_version = HPPA_CPU_PCXUP;
	}

	/* locate coprocessors and SFUs */
	memset(&pdc_coproc, 0, sizeof(pdc_coproc));
	error = pdcproc_coproc(&pdc_coproc);
	if (error < 0) {
		DPRINTF(("WARNING: PDC_COPROC error %d\n", error));
		pdc_coproc.ccr_enable = 0;
	} else {
		DPRINTF(("pdc_coproc: 0x%x, 0x%x; model %x rev %x\n",
		    pdc_coproc.ccr_enable, pdc_coproc.ccr_present,
		    pdc_coproc.fpu_model, pdc_coproc.fpu_revision));

		/* a kludge to detect PCXW */
		if (pdc_coproc.fpu_model == HPPA_FPU_PCXW)
			cpu_version = HPPA_CPU_PCXW;
	}
	mtctl(pdc_coproc.ccr_enable & CCR_MASK, CR_CCR);
	DPRINTF(("%s: bootstrap fpu\n", __func__));

	usebtlb = 0;
	if (cpu_version == HPPA_CPU_PCXW || cpu_version > HPPA_CPU_PCXL2) {
		DPRINTF(("WARNING: BTLB no supported on cpu %d\n", cpu_version));
	} else {

		/* BTLB params */
		error = pdcproc_block_tlb(&pdc_btlb);
		if (error < 0) {
			DPRINTF(("WARNING: PDC_BTLB error %d\n", error));
		} else {
			DPRINTFN(10, ("btlb info: minsz=%d, maxsz=%d\n",
			    pdc_btlb.min_size, pdc_btlb.max_size));
			DPRINTFN(10, ("btlb fixed: i=%d, d=%d, c=%d\n",
			    pdc_btlb.finfo.num_i,
			    pdc_btlb.finfo.num_d,
			    pdc_btlb.finfo.num_c));
			DPRINTFN(10, ("btlb varbl: i=%d, d=%d, c=%d\n",
			    pdc_btlb.vinfo.num_i,
			    pdc_btlb.vinfo.num_d,
			    pdc_btlb.vinfo.num_c));

			/* purge TLBs and caches */
			if (pdcproc_btlb_purgeall() < 0)
				DPRINTFN(10, ("WARNING: BTLB purge failed\n"));

			hppa_btlb_size_min = pdc_btlb.min_size;
			hppa_btlb_size_max = pdc_btlb.max_size;

			DPRINTF(("hppa_btlb_size_min 0x%x\n", hppa_btlb_size_min));
			DPRINTF(("hppa_btlb_size_max 0x%x\n", hppa_btlb_size_max));

			if (pdc_btlb.finfo.num_c)
				cpu_features |= HPPA_FTRS_BTLBU;
			usebtlb = 1;
		}
	}
	usebtlb = 0;

	error = pdcproc_tlb_info(&pdc_hwtlb);
	if (error == 0 && pdc_hwtlb.min_size != 0 && pdc_hwtlb.max_size != 0) {
		cpu_features |= HPPA_FTRS_HVT;
		if (pmap_hptsize > pdc_hwtlb.max_size)
			pmap_hptsize = pdc_hwtlb.max_size;
		else if (pmap_hptsize && pmap_hptsize < pdc_hwtlb.min_size)
			pmap_hptsize = pdc_hwtlb.min_size;

		DPRINTF(("%s: pmap_hptsize 0x%x\n", __func__, pmap_hptsize));
	} else {
		DPRINTF(("WARNING: no HPT support, fine!\n"));

		pmap_hptsize = 0;
	}

	bool cpu_found = false;
	if (cpu_version) {
		DPRINTF(("%s: looking for cpu_version %x\n", __func__,
		    cpu_version));
		for (i = 0, p = cpu_types; i < __arraycount(cpu_types);
		     i++, p++) {
			if (p->hci_cpuversion == cpu_version) {
				cpu_found = true;
				break;
			}
		}
	} else if (cpu_type != hpc_unknown) {
		DPRINTF(("%s: looking for cpu_type %d\n", __func__,
		    cpu_type));
		for (i = 0, p = cpu_types; i < __arraycount(cpu_types);
		     i++, p++) {
			if (p->hci_cputype == cpu_type) {
				cpu_found = true;
				break;
			}
		}
	}

	if (!cpu_found) {
		panic("CPU detection failed. Please report the problem.");
	}

	hppa_cpu_info = p;

	if (hppa_cpu_info->hci_chip_name == NULL)
		panic("bad model string for 0x%x", pdc_model.hwmodel);

	/*
	 * TODO: HPT on 7200 is not currently supported
	 */
	if (pmap_hptsize && p->hci_cputype != hpcxl && p->hci_cputype != hpcxl2)
		pmap_hptsize = 0;

	cpu_type = hppa_cpu_info->hci_cputype;
	cpu_ibtlb_ins = hppa_cpu_info->ibtlbins;
	cpu_dbtlb_ins = hppa_cpu_info->dbtlbins;
	cpu_hpt_init = hppa_cpu_info->hptinit;
	cpu_desidhash = hppa_cpu_info->desidhash;

	if (cpu_desidhash)
		cpu_revision = (*cpu_desidhash)();
	else
		cpu_revision = 0;

	/* force strong ordering for now */
	if (hppa_cpu_ispa20_p())
		curcpu()->ci_psw |= PSW_O;

	cpu_setmodel("HP9000/%s", model);

#define	LDILDO(t,f) ((t)[0] = (f)[0], (t)[1] = (f)[1]);
	LDILDO(trap_ep_T_TLB_DIRTY , hppa_cpu_info->tlbdh);
	LDILDO(trap_ep_T_DTLBMISS  , hppa_cpu_info->dtlbh);
	LDILDO(trap_ep_T_DTLBMISSNA, hppa_cpu_info->dtlbnah);
	LDILDO(trap_ep_T_ITLBMISS  , hppa_cpu_info->itlbh);
	LDILDO(trap_ep_T_ITLBMISSNA, hppa_cpu_info->itlbnah);
#undef LDILDO

	/* Bootstrap any FPU. */
	hppa_fpu_bootstrap(pdc_coproc.ccr_enable);
}

enum hppa_cpu_type
cpu_model_cpuid(int modelno)
{
	switch (modelno) {
	/* no supported HP8xx/9xx models with pcx */
	case HPPA_BOARD_HP720:
	case HPPA_BOARD_HP750_66:
	case HPPA_BOARD_HP730_66:
	case HPPA_BOARD_HP710:
	case HPPA_BOARD_HP705:
		return hpcxs;

	case HPPA_BOARD_HPE23:
	case HPPA_BOARD_HPE25:
	case HPPA_BOARD_HPE35:
	case HPPA_BOARD_HPE45:
	case HPPA_BOARD_HP712_60:
	case HPPA_BOARD_HP712_80:
	case HPPA_BOARD_HP712_100:
	case HPPA_BOARD_HP715_80:
	case HPPA_BOARD_HP715_64:
	case HPPA_BOARD_HP715_100:
	case HPPA_BOARD_HP715_100XC:
	case HPPA_BOARD_HP715_100L:
	case HPPA_BOARD_HP715_120L:
	case HPPA_BOARD_HP715_80M:
		return hpcxl;

	case HPPA_BOARD_HP735_99:
	case HPPA_BOARD_HP755_99:
	case HPPA_BOARD_HP755_125:
	case HPPA_BOARD_HP735_130:
	case HPPA_BOARD_HP715_50:
	case HPPA_BOARD_HP715_33:
	case HPPA_BOARD_HP715S_50:
	case HPPA_BOARD_HP715S_33:
	case HPPA_BOARD_HP715T_50:
	case HPPA_BOARD_HP715T_33:
	case HPPA_BOARD_HP715_75:
	case HPPA_BOARD_HP715_99:
	case HPPA_BOARD_HP725_50:
	case HPPA_BOARD_HP725_75:
	case HPPA_BOARD_HP725_99:
		return hpcxt;
	}
	return hpc_unknown;
}

void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
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
	printf("%s%s", copyright, version);

	/* identify system type */
	printf("%s\n", cpu_getmodel());

	/* Display some memory usage information. */
	format_bytes(pbuf[0], sizeof(pbuf[0]), ptoa(physmem));
	format_bytes(pbuf[1], sizeof(pbuf[1]), ptoa(resvmem));
	format_bytes(pbuf[2], sizeof(pbuf[2]), ptoa(availphysmem));
	printf("real mem = %s (%s reserved for PROM, %s used by NetBSD)\n",
	    pbuf[0], pbuf[1], pbuf[2]);

#ifdef DEBUG
	if (totalphysmem > physmem) {
		format_bytes(pbuf[0], sizeof(pbuf[0]), ptoa(totalphysmem - physmem));
		DPRINTF(("lost mem = %s\n", pbuf[0]));
	}
#endif

	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);

#ifdef PMAPDEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf[0], sizeof(pbuf[0]), ptoa(uvmexp.free));
	printf("avail mem = %s\n", pbuf[0]);
}

/*
 * compute CPU clock ratio such as:
 *	cpu_ticksnum / cpu_ticksdenom = t + delta
 *	delta -> 0
 */
void
delay_init(void)
{
	u_int num, denom, delta, mdelta;

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
			mdelta = delta;
		}
	}
}

void
delay(u_int us)
{
	u_int start, end, n;

	mfctl(CR_ITMR, start);
	while (us) {
		n = uimin(1000, us);
		end = start + n * cpu_ticksnum / cpu_ticksdenom;

		/* N.B. Interval Timer may wrap around */
		if (end < start) {
			do {
				mfctl(CR_ITMR, start);
			} while (start > end);
		}

		do
			mfctl(CR_ITMR, start);
		while (start < end);

		us -= n;
		mfctl(CR_ITMR, start);
	}
}

static inline void
fall(int c_base, int c_count, int c_loop, int c_stride, int data)
{
	int loop;

	for (; c_count--; c_base += c_stride)
		for (loop = c_loop; loop--; )
			if (data)
				fdce(0, c_base);
			else
				fice(0, c_base);
}

void
fcacheall(void)
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
ptlball(void)
{
	pa_space_t sp;
	int i, j, k;

	/* instruction TLB */
	sp = pdc_cache.it_sp_base;
	for (i = 0; i < pdc_cache.it_sp_count; i++) {
		vaddr_t off = pdc_cache.it_off_base;
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
		vaddr_t off = pdc_cache.dt_off_base;
		for (j = 0; j < pdc_cache.dt_off_count; j++) {
			for (k = 0; k < pdc_cache.dt_loop; k++)
				pdtlbe(sp, off);
			off += pdc_cache.dt_off_stride;
		}
		sp += pdc_cache.dt_sp_stride;
	}
}

int
hpti_g(vaddr_t hpt, vsize_t hptsize)
{

	return pdcproc_tlb_config(&pdc_hwtlb, hpt, hptsize, PDC_TLB_CURRPDE);
}

int
pbtlb_g(int i)
{
	return -1;
}

int
ibtlb_g(int i, pa_space_t sp, vaddr_t va, paddr_t pa, vsize_t sz, u_int prot)
{
	int error;

	error = pdcproc_btlb_insert(sp, va, pa, sz, prot, i);
	if (error < 0) {
#ifdef BTLBDEBUG
		DPRINTF(("WARNING: BTLB insert failed (%d)\n", error));
#endif
	}
	return error;
}


/*
 * This inserts a recorded BTLB slot.
 */
static int _hppa_btlb_insert(struct btlb_slot *);
static int
_hppa_btlb_insert(struct btlb_slot *btlb_slot)
{
	int error;
#ifdef MACHDEPDEBUG
	const char *prot;

	/* Display the protection like a file protection. */
	switch (btlb_slot->btlb_slot_tlbprot & TLB_AR_MASK) {
	case TLB_AR_NA:			prot = "------"; break;
	case TLB_AR_R:			prot = "r-----"; break;
	case TLB_AR_RW:			prot = "rw----"; break;
	case TLB_AR_RX:			prot = "r-x---"; break;
	case TLB_AR_RWX:		prot = "rwx---"; break;
	case TLB_AR_R | TLB_USER:	prot = "r--r--"; break;
	case TLB_AR_RW | TLB_USER:	prot = "rw-rw-"; break;
	case TLB_AR_RX | TLB_USER:	prot = "r--r-x"; break;
	case TLB_AR_RWX | TLB_USER:	prot = "rw-rwx"; break;
	default:		prot = "??????"; break;
	}

	DPRINTFN(10, (
	    "  [ BTLB %d: %s 0x%08x @ 0x%x:0x%08x len 0x%08x prot 0x%08x]  ",
	    btlb_slot->btlb_slot_number,
	    prot,
	    (u_int)btlb_slot->btlb_slot_pa_frame << PGSHIFT,
	    btlb_slot->btlb_slot_va_space,
	    (u_int)btlb_slot->btlb_slot_va_frame << PGSHIFT,
	    (u_int)btlb_slot->btlb_slot_frames << PGSHIFT,
	    btlb_slot->btlb_slot_tlbprot));

	/*
	 * Non-I/O space mappings are entered by the pmap,
	 * so we do print a newline to make things look better.
	 */
	if (btlb_slot->btlb_slot_pa_frame < (HPPA_IOSPACE >> PGSHIFT))
		DPRINTFN(10, ("\n"));
#endif

	/* Insert this mapping. */
	error = pdcproc_btlb_insert(
		btlb_slot->btlb_slot_va_space,
		btlb_slot->btlb_slot_va_frame,
		btlb_slot->btlb_slot_pa_frame,
		btlb_slot->btlb_slot_frames,
		btlb_slot->btlb_slot_tlbprot,
		btlb_slot->btlb_slot_number);
	if (error < 0) {
#ifdef BTLBDEBUG
		DPRINTF(("WARNING: BTLB insert failed (%d)\n", error);
#endif
	}
	return (error ? EINVAL : 0);
}

/*
 * This records and inserts a new BTLB entry.
 */
int
hppa_btlb_insert(pa_space_t space, vaddr_t va, paddr_t pa, vsize_t *sizep,
    u_int tlbprot)
{
	struct btlb_slot *btlb_slot, *btlb_slot_best, *btlb_slot_end;
	vsize_t frames;
	int error;
	int need_dbtlb, need_ibtlb, need_variable_range;
	int btlb_slot_score, btlb_slot_best_score;
	vsize_t slot_mapped_frames, total_mapped_frames;

	/*
	 * All entries need data translation.  Those that
	 * allow execution also need instruction translation.
	 */
	switch (tlbprot & TLB_AR_MASK) {
	case TLB_AR_R:
	case TLB_AR_RW:
	case TLB_AR_R | TLB_USER:
	case TLB_AR_RW | TLB_USER:
		need_dbtlb = true;
		need_ibtlb = false;
		break;
	case TLB_AR_RX:
	case TLB_AR_RWX:
	case TLB_AR_RX | TLB_USER:
	case TLB_AR_RWX | TLB_USER:
		need_dbtlb = true;
		need_ibtlb = true;
		break;
	default:
		panic("btlb_insert: bad tlbprot");
	}

	/*
	 * If this entry isn't aligned to the size required
	 * for a fixed-range slot, it requires a variable-range
	 * slot.  This also converts pa and va to page frame
	 * numbers.
	 */
	frames = pdc_btlb.min_size << PGSHIFT;
	while (frames < *sizep)
		frames <<= 1;
	frames >>= PGSHIFT;
	if (frames > pdc_btlb.max_size) {
#ifdef BTLBDEBUG
		DPRINTF(("btlb_insert: too big (%u < %u < %u)\n",
		    pdc_btlb.min_size, (u_int) frames, pdc_btlb.max_size);
#endif
		return -(ENOMEM);
	}
	pa >>= PGSHIFT;
	va >>= PGSHIFT;
	need_variable_range =
		((pa & (frames - 1)) != 0 || (va & (frames - 1)) != 0);

	/* I/O space must be mapped uncached. */
	if (pa >= HPPA_IOBEGIN)
		tlbprot |= TLB_UNCACHEABLE;

	/*
	 * Loop while we still need slots.
	 */
	btlb_slot_end = btlb_slots + btlb_slots_count;
	total_mapped_frames = 0;
	btlb_slot_best_score = 0;
	while (need_dbtlb || need_ibtlb) {

		/*
		 * Find an applicable slot.
		 */
		btlb_slot_best = NULL;
		for (btlb_slot = btlb_slots;
		     btlb_slot < btlb_slot_end;
		     btlb_slot++) {

			/*
			 * Skip this slot if it's in use, or if we need a
			 * variable-range slot and this isn't one.
			 */
			if (btlb_slot->btlb_slot_frames != 0 ||
			    (need_variable_range &&
			     !(btlb_slot->btlb_slot_flags &
			       BTLB_SLOT_VARIABLE_RANGE)))
				continue;

			/*
			 * Score this slot.
			 */
			btlb_slot_score = 0;
			if (need_dbtlb &&
			    (btlb_slot->btlb_slot_flags & BTLB_SLOT_DBTLB))
				btlb_slot_score++;
			if (need_ibtlb &&
			    (btlb_slot->btlb_slot_flags & BTLB_SLOT_IBTLB))
				btlb_slot_score++;

			/*
			 * Update the best slot.
			 */
			if (btlb_slot_score > 0 &&
			    (btlb_slot_best == NULL ||
			     btlb_slot_score > btlb_slot_best_score)) {
				btlb_slot_best = btlb_slot;
				btlb_slot_best_score = btlb_slot_score;
			}
		}

		/*
		 * If there were no applicable slots.
		 */
		if (btlb_slot_best == NULL) {
			DPRINTFN(10, ("BTLB full\n"));
			return -(ENOMEM);
		}

		/*
		 * Now fill this BTLB slot record and insert the entry.
		 */
		if (btlb_slot->btlb_slot_flags & BTLB_SLOT_VARIABLE_RANGE)
			slot_mapped_frames = ((*sizep + PGOFSET) >> PGSHIFT);
		else
			slot_mapped_frames = frames;
		if (slot_mapped_frames > total_mapped_frames)
			total_mapped_frames = slot_mapped_frames;
		btlb_slot = btlb_slot_best;
		btlb_slot->btlb_slot_va_space = space;
		btlb_slot->btlb_slot_va_frame = va;
		btlb_slot->btlb_slot_pa_frame = pa;
		btlb_slot->btlb_slot_tlbprot = tlbprot;
		btlb_slot->btlb_slot_frames = slot_mapped_frames;
		error = _hppa_btlb_insert(btlb_slot);
		if (error)
			return -error;
		/*
		 * Note what slots we no longer need.
		 */
		if (btlb_slot->btlb_slot_flags & BTLB_SLOT_DBTLB)
			need_dbtlb = false;
		if (btlb_slot->btlb_slot_flags & BTLB_SLOT_IBTLB)
			need_ibtlb = false;
	}

	/* Success. */
	*sizep = (total_mapped_frames << PGSHIFT);
	return 0;
}

/*
 * This reloads the BTLB in the event that it becomes invalidated.
 */
int
hppa_btlb_reload(void)
{
	struct btlb_slot *btlb_slot, *btlb_slot_end;
	int error;

	/* Insert all recorded BTLB entries. */
	btlb_slot = btlb_slots;
	btlb_slot_end = btlb_slots + btlb_slots_count;
	error = 0;
	while (error == 0 && btlb_slot < btlb_slot_end) {
		if (btlb_slot->btlb_slot_frames != 0)
			error = _hppa_btlb_insert(btlb_slot);
		btlb_slot++;
	}
	DPRINTF(("\n"));
	return (error);
}

/*
 * This purges a BTLB entry.
 */
int
hppa_btlb_purge(pa_space_t space, vaddr_t va, vsize_t *sizep)
{
	struct btlb_slot *btlb_slot, *btlb_slot_end;
	int error;

	/*
	 * Purge all slots that map this virtual address.
	 */
	error = ENOENT;
	va >>= PGSHIFT;
	btlb_slot_end = btlb_slots + btlb_slots_count;
	for (btlb_slot = btlb_slots;
	     btlb_slot < btlb_slot_end;
	     btlb_slot++) {
		if (btlb_slot->btlb_slot_frames != 0 &&
		    btlb_slot->btlb_slot_va_space == space &&
		    btlb_slot->btlb_slot_va_frame == va) {
			error = pdcproc_btlb_purge(
				btlb_slot->btlb_slot_va_space,
				btlb_slot->btlb_slot_va_frame,
				btlb_slot->btlb_slot_number,
				btlb_slot->btlb_slot_frames);
			if (error < 0) {
				DPRINTFN(10, ("WARNING: BTLB purge failed (%d)\n",
					error));

				return (error);
			}

			/*
			 * Tell our caller how many bytes were mapped
			 * by this slot, then free the slot.
			 */
			*sizep = (btlb_slot->btlb_slot_frames << PGSHIFT);
			btlb_slot->btlb_slot_frames = 0;
		}
	}
	return (error);
}

/*
 * This maps page zero if it isn't already mapped, and
 * returns a cookie for hppa_pagezero_unmap.
 */
int
hppa_pagezero_map(void)
{
	int was_mapped_before;
	int s;

	was_mapped_before = pagezero_mapped;
	if (!was_mapped_before) {
		s = splhigh();
		pmap_kenter_pa(0, 0, VM_PROT_ALL, 0);
		pagezero_mapped = 1;
		splx(s);
	}
	return (was_mapped_before);
}

/*
 * This unmaps mape zero, given a cookie previously returned
 * by hppa_pagezero_map.
 */
void
hppa_pagezero_unmap(int was_mapped_before)
{
	int s;

	if (!was_mapped_before) {
		s = splhigh();
		pmap_kremove(0, PAGE_SIZE);
		pagezero_mapped = 0;
		splx(s);
	}
}

int waittime = -1;

__dead void
cpu_reboot(int howto, char *user_boot_string)
{
	boothowto = howto | (boothowto & RB_HALT);

	if (!(howto & RB_NOSYNC) && waittime < 0) {
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* XXX probably save howto into stable storage */

	/* Disable interrupts. */
	splhigh();

	/* Make a crash dump. */
	if (howto & RB_DUMP)
		dumpsys();

	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	/* in case we came on powerfail interrupt */
	if (cold_hook)
		(*cold_hook)(HPPA_COLD_COLD);

	if (howto & RB_HALT) {
		if ((howto & RB_POWERDOWN) == RB_POWERDOWN && cold_hook) {
			printf("Powering off...");
			DELAY(1000000);
			(*cold_hook)(HPPA_COLD_OFF);
			DELAY(1000000);
		}

		printf("System halted!\n");
		DELAY(1000000);
		__asm volatile("stwas %0, 0(%1)"
		    :: "r" (CMD_STOP), "r" (LBCAST_ADDR + iomod_command));
	} else {
		printf("rebooting...");
		DELAY(1000000);
		__asm volatile("stwas %0, 0(%1)"
		    :: "r" (CMD_RESET), "r" (LBCAST_ADDR + iomod_command));

		/* ask firmware to reset */
		pdcproc_doreset();
		/* forcably reset module if that fails */
		__asm __volatile("stwas %0, 0(%1)"
		    :: "r" (CMD_RESET), "r" (HPPA_LBCAST + iomod_command));
	}

	for (;;) {
		/*
		 * loop while bus reset is coming up.  This NOP instruction
		 * is used by qemu to detect the 'death loop'.
		 */
		__asm volatile("or %%r31, %%r31, %%r31" ::: "memory");
	}
	/* NOTREACHED */
}

uint32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
int
cpu_dumpsize(void)
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
static char in_check = 0;

#define	PIM_WORD(name, word, bits)			\
do {							\
	snprintb(bitmask_buffer, sizeof(bitmask_buffer),\
	    bits, word);				\
	printf("%s %s", name, bitmask_buffer);		\
} while (/* CONSTCOND */ 0)


static inline void
hppa_pim_dump(int check_type, void *data, size_t size)
{
	struct hppa_pim_hpmc *hpmc;
	struct hppa_pim_lpmc *lpmc;
	struct hppa_pim_toc *toc;
	struct hppa_pim_regs *regs;
	struct hppa_pim_checks *checks;
	u_int *regarray;
	int reg_i, reg_j, reg_k;
	char bitmask_buffer[64];
	const char *name;

	regs = NULL;
	checks = NULL;
	switch (check_type) {
	case T_HPMC:
		hpmc = (struct hppa_pim_hpmc *) data;
		regs = &hpmc->pim_hpmc_regs;
		checks = &hpmc->pim_hpmc_checks;
		break;
	case T_LPMC:
		lpmc = (struct hppa_pim_lpmc *) data;
		checks = &lpmc->pim_lpmc_checks;
		break;
	case T_INTERRUPT:
		toc = (struct hppa_pim_toc *) data;
		regs = &toc->pim_toc_regs;
		break;
	default:
		panic("unknown machine check type");
		/* NOTREACHED */
	}

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
				    (reg_k & 3) ? " " : "\n",
				    regarray[reg_k]);
		}

		/* Print out some interesting registers. */
		printf("\n\n\tIIA head 0x%x:0x%08x\n"
			"\tIIA tail 0x%x:0x%08x",
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
}

static inline void
hppa_pim64_dump(int check_type, void *data, size_t size)
{
	struct hppa_pim64_hpmc *hpmc;
	struct hppa_pim64_lpmc *lpmc;
	struct hppa_pim64_toc *toc;
	struct hppa_pim64_regs *regs;
	struct hppa_pim64_checks *checks;
	int reg_i, reg_j, reg_k;
	uint64_t *regarray;
	char bitmask_buffer[64];
	const char *name;

	regs = NULL;
	checks = NULL;
	switch (check_type) {
	case T_HPMC:
		hpmc = (struct hppa_pim64_hpmc *) data;
		regs = &hpmc->pim_hpmc_regs;
		checks = &hpmc->pim_hpmc_checks;
		break;
	case T_LPMC:
		lpmc = (struct hppa_pim64_lpmc *) data;
		checks = &lpmc->pim_lpmc_checks;
		break;
	case T_INTERRUPT:
		toc = (struct hppa_pim64_toc *) data;
		regs = &toc->pim_toc_regs;
		break;
	default:
		panic("unknown machine check type");
		/* NOTREACHED */
	}

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
			printf("\n\n%s Registers:", name);
			for (reg_k = 0; reg_k < reg_j; reg_k++)
				printf("%s0x%016lx",
				   (reg_k & 3) ? " " : "\n",
				   (unsigned long)regarray[reg_k]);
		}

		/* Print out some interesting registers. */
		printf("\n\nIIA head 0x%lx:0x%016lx\n"
	            "IIA tail 0x%lx:0x%016lx",
		    (unsigned long)regs->pim_regs_cr17,
		    (unsigned long)regs->pim_regs_cr18,
		    (unsigned long)regs->pim_regs_iisq_tail,
		    (unsigned long)regs->pim_regs_iioq_tail);
		PIM_WORD("\nIPSW", regs->pim_regs_cr22, PSW_BITS);
		printf("\nSP 0x%lx:0x%016lx\nFP 0x%lx:0x%016lx",
       		    (unsigned long)regs->pim_regs_sr0,
		    (unsigned long)regs->pim_regs_r30,
		    (unsigned long)regs->pim_regs_sr0,
		    (unsigned long)regs->pim_regs_r3);
	}

	/* If we have check words, display them. */
	if (checks != NULL) {
		PIM_WORD("\n\nCheck Type", checks->pim_check_type,
			PIM_CHECK_BITS);
		PIM_WORD("\nCPU State", checks->pim_check_cpu_state,
			PIM_CPU_BITS PIM_CPU_HPMC_BITS);
		PIM_WORD("\nCache Check", checks->pim_check_cache,
			PIM_CACHE_BITS);
		PIM_WORD("\nTLB Check", checks->pim_check_tlb,
			PIM_TLB_BITS);
		PIM_WORD("\nBus Check", checks->pim_check_bus,
			PIM_BUS_BITS);
		PIM_WORD("\nAssist Check", checks->pim_check_assist,
			PIM_ASSIST_BITS);
		printf("\nAssist State %u", checks->pim_check_assist_state);
		printf("\nSystem Responder 0x%016lx",
		        (unsigned long)checks->pim_check_responder);
		printf("\nSystem Requestor 0x%016lx",
		        (unsigned long)checks->pim_check_requestor);
		printf("\nPath Info 0x%08x",
		        checks->pim_check_path_info);
	}
}

void
hppa_machine_check(int check_type)
{
	int pdc_pim_type;
	const char *name;
	int pimerror, error;
	void *data;
	size_t size;

	/* Do an fcacheall(). */
	fcacheall();

	/* Dispatch on the check type. */
	switch (check_type) {
	case T_HPMC:
		name = "HPMC";
		pdc_pim_type = PDC_PIM_HPMC;
		break;
	case T_LPMC:
		name = "LPMC";
		pdc_pim_type = PDC_PIM_LPMC;
		break;
	case T_INTERRUPT:
		name = "TOC";
		pdc_pim_type = PDC_PIM_TOC;
		break;
	default:
		panic("unknown machine check type");
		/* NOTREACHED */
	}

	pimerror = pdcproc_pim(pdc_pim_type, &pdc_pim, &data, &size);

	KASSERT(pdc_pim.count <= size);

	/*
	 * Reset IO and log errors.
	 *
	 * This seems to be needed in order to output to the console
	 * if we take a HPMC interrupt. This PDC procedure may not be
	 * implemented by some machines.
	 */
	error = pdcproc_ioclrerrors();
	if (error != PDC_ERR_OK && error != PDC_ERR_NOPROC)
		/* This seems futile if we can't print to the console. */
		panic("PDC_IO failed");

	printf("\nmachine check: %s", name);

	if (pimerror < 0) {
		printf(" - WARNING: could not transfer PIM info (%d)", pimerror);
	} else {
		if (hppa_cpu_ispa20_p())
			hppa_pim64_dump(check_type, data, size);
		else
			hppa_pim_dump(check_type, data, size);
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
cpu_dump(void)
{
	long buf[dbtob(1) / sizeof (long)];
	kcore_seg_t	*segp;
	cpu_kcore_hdr_t	*cpuhdrp __unused;
	const struct bdevsw *bdev;

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

	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return (-1);

	return (*bdev->d_dump)(dumpdev, dumplo, (void *)buf, dbtob(1));
}

/*
 * Dump the kernel's image to the swap partition.
 */
#define	BYTES_PER_DUMP	PAGE_SIZE

void
dumpsys(void)
{
	const struct bdevsw *bdev;
	int psize, bytes, i, n;
	char *maddr;
	daddr_t blkno;
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int error;

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return;

	/* Save registers
	savectx(&dumppcb); */

	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n",
		    major(dumpdev), minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n",
	    major(dumpdev), minor(dumpdev), dumplo);

	psize = bdev_size(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	if (!(error = cpu_dump())) {

		/* XXX fredette - this is way broken: */
		bytes = ctob(physmem);
		maddr = NULL;
		blkno = dumplo + cpu_dumpsize();
		dump = bdev->d_dump;
		/* TODO block map the whole physical memory */
		for (i = 0; i < bytes; i += n) {

			/* Print out how many MBs we are to go. */
			n = bytes - i;
			if (n && (n % (1024*1024)) == 0)
				printf_nolog("%d ", n / (1024 * 1024));

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

void
hppa_setvmspace(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct trapframe *tf = l->l_md.md_regs;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	pa_space_t space = pmap->pm_space;

    	if (p->p_md.md_flags & MDP_OLDSPACE) {
		tf->tf_sr7 = HPPA_SID_KERNEL;
	} else {
		tf->tf_sr7 = space;
	}

	tf->tf_sr2 = HPPA_SID_KERNEL;

	/* Load all of the user's space registers. */
	tf->tf_sr0 = tf->tf_sr1 = tf->tf_sr3 =
	tf->tf_sr4 = tf->tf_sr5 = tf->tf_sr6 =
	tf->tf_iisq_head = tf->tf_iisq_tail = space;

	/* Load the protection registers. */
	tf->tf_pidr1 = tf->tf_pidr2 = pmap->pm_pid;
}

/*
 * Set registers on exec.
 */
void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct proc *p = l->l_proc;
	struct trapframe *tf = l->l_md.md_regs;
	struct pcb *pcb = lwp_getpcb(l);

	tf->tf_flags = TFF_SYS|TFF_LAST;
	tf->tf_iioq_tail = 4 +
	    (tf->tf_iioq_head = pack->ep_entry | HPPA_PC_PRIV_USER);
	tf->tf_rp = 0;
	tf->tf_arg0 = p->p_psstrp;
	tf->tf_arg1 = tf->tf_arg2 = 0; /* XXX dynload stuff */

	if (pack->ep_osversion < 699003600) {
		p->p_md.md_flags |= MDP_OLDSPACE;
	} else {
		p->p_md.md_flags = 0;
	}

	hppa_setvmspace(l);

	/* reset any of the pending FPU exceptions */
	hppa_fpu_flush(l);
	pcb->pcb_fpregs->fpr_regs[0] = ((uint64_t)HPPA_FPU_INIT) << 32;
	pcb->pcb_fpregs->fpr_regs[1] = 0;
	pcb->pcb_fpregs->fpr_regs[2] = 0;
	pcb->pcb_fpregs->fpr_regs[3] = 0;

	l->l_md.md_bpva = 0;

	/* setup terminal stack frame */
	stack = (u_long)STACK_ALIGN(stack, 63);
	tf->tf_r3 = stack;
	ustore_long((void *)(stack), 0);
	stack += HPPA_FRAME_SIZE;
	ustore_long((void *)(stack + HPPA_FRAME_PSP), 0);
	tf->tf_sp = stack;
}

/*
 * machine dependent system variables.
 */
static int
sysctl_machdep_boot(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct btinfo_kernelfile *bi_file;
	const char *cp = NULL;

	switch (node.sysctl_num) {
	case CPU_BOOTED_KERNEL:
		if ((bi_file = lookup_bootinfo(BTINFO_KERNELFILE)) != NULL)
			cp = bi_file->name;
		if (cp != NULL && cp[0] == '\0')
			cp = "netbsd";
		break;
	default:
		return (EINVAL);
	}

	if (cp == NULL || cp[0] == '\0')
		return (ENOENT);

	node.sysctl_data = __UNCONST(cp);
	node.sysctl_size = strlen(cp) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

#if NLCD > 0
static int
sysctl_machdep_heartbeat(SYSCTLFN_ARGS)
{
	int error;
	bool oldval;
	struct sysctlnode node = *rnode;

	oldval = lcd_blink_p;
	/*
	 * If we were false and are now true, start the timer.
	 */
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return (error);

	if (!oldval && lcd_blink_p)
		blink_lcd_timeout(NULL);

	return 0;
}
#endif

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_boot, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
#if NLCD > 0
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "lcd_blink", "Display heartbeat on the LCD display",
		       sysctl_machdep_heartbeat, 0, &lcd_blink_p, 0,
		       CTL_MACHDEP, CPU_LCD_BLINK, CTL_EOL);
#endif
}

/*
 * Given the type of a bootinfo entry, looks for a matching item inside
 * the bootinfo structure.  If found, returns a pointer to it (which must
 * then be casted to the appropriate bootinfo_* type); otherwise, returns
 * NULL.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bic;
	int i;

	bic = (struct btinfo_common *)(&bootinfo.bi_data[0]);
	for (i = 0; i < bootinfo.bi_nentries; i++)
		if (bic->type == type)
			return bic;
		else
			bic = (struct btinfo_common *)
			    ((uint8_t *)bic + bic->len);

	return NULL;
}

/*
 * consinit:
 * initialize the system console.
 */
void
consinit(void)
{
	static int initted = 0;

	if (!initted) {
		initted++;
		cninit();
	}
}

#if NLCD > 0
struct blink_lcd_softc {
	SLIST_HEAD(, blink_lcd) bls_head;
	int bls_on;
	struct callout bls_to;
} blink_sc = {
	.bls_head = SLIST_HEAD_INITIALIZER(bls_head)
};

void
blink_lcd_register(struct blink_lcd *l)
{
	if (SLIST_EMPTY(&blink_sc.bls_head)) {
		callout_init(&blink_sc.bls_to, 0);
		callout_setfunc(&blink_sc.bls_to, blink_lcd_timeout, &blink_sc);
		blink_sc.bls_on = 0;
		if (lcd_blink_p)
			callout_schedule(&blink_sc.bls_to, 1);
	}
	SLIST_INSERT_HEAD(&blink_sc.bls_head, l, bl_next);
}

void
blink_lcd_timeout(void *vsc)
{
	struct blink_lcd_softc *sc = &blink_sc;
	struct blink_lcd *l;
	int t;

	if (SLIST_EMPTY(&sc->bls_head))
		return;

	SLIST_FOREACH(l, &sc->bls_head, bl_next) {
		(*l->bl_func)(l->bl_arg, sc->bls_on);
	}
	sc->bls_on = !sc->bls_on;

	if (!lcd_blink_p)
		return;

	/*
	 * Blink rate is:
	 *      full cycle every second if completely idle (loadav = 0)
	 *      full cycle every 2 seconds if loadav = 1
	 *      full cycle every 3 seconds if loadav = 2
	 * etc.
	 */
	t = (((averunnable.ldavg[0] + FSCALE) * hz) >> (FSHIFT + 1));
	callout_schedule(&sc->bls_to, t);
}
#endif

#ifdef MODULAR
/*
 * Push any modules loaded by the boot loader.
 */
void
module_init_md(void)
{
}
#endif /* MODULAR */

bool
mm_md_direct_mapped_phys(paddr_t paddr, vaddr_t *vaddr)
{

	if (atop(paddr) > physmem) {
		return false;
	}
	*vaddr = paddr;

	return true;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (atop(pa) > physmem) ? EFAULT : 0;
}

int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{
	extern int kernel_text;
	extern int __data_start;
	extern int end;

	const vaddr_t ksro = (vaddr_t) &kernel_text;
	const vaddr_t ksrw = (vaddr_t) &__data_start;
	const vaddr_t kend = (vaddr_t) end;
	const vaddr_t v = (vaddr_t)ptr;

	*handled = false;
	if (v >= ksro && v < kend) {
		*handled = true;
		if (v < ksrw && (prot & VM_PROT_WRITE)) {
			return EFAULT;
		}
	} else if (v >= kend && atop((paddr_t)v) < physmem) {
		*handled = true;
	}

	return 0;
}
