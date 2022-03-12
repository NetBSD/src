/* $NetBSD: aarch64_machdep.c,v 1.65 2022/03/12 09:16:05 skrll Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aarch64_machdep.c,v 1.65 2022/03/12 09:16:05 skrll Exp $");

#include "opt_arm_debug.h"
#include "opt_cpuoptions.h"
#include "opt_ddb.h"
#include "opt_fdt.h"
#include "opt_kernhist.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/asan.h>
#include <sys/boot_flag.h>
#include <sys/bus.h>
#include <sys/core.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/kcore.h>
#include <sys/module.h>
#include <sys/msgbuf.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <dev/mm.h>

#include <uvm/uvm.h>

#include <machine/bootconfig.h>

#include <arm/cpufunc.h>

#include <aarch64/armreg.h>
#ifdef DDB
#include <aarch64/db_machdep.h>
#endif
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/pmap.h>
#include <aarch64/pte.h>
#include <aarch64/vmparam.h>
#include <aarch64/kcore.h>

#include <arm/fdt/arm_fdtvar.h>
#include <dev/fdt/fdt_memory.h>

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

char cpu_model[32];
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
	[PCU_FPU] = &pcu_fpu_ops
};

struct vm_map *phys_map;

#ifdef MODULAR
vaddr_t module_start, module_end;
static struct vm_map module_map_store;
#endif

#ifdef KASAN
vaddr_t kasan_kernelstart;
vaddr_t kasan_kernelsize;
#endif

/* XXX */
vaddr_t physical_start;
vaddr_t physical_end;
/* filled in before cleaning bss. keep in .data */
u_long kern_vtopdiff __attribute__((__section__(".data")));

/* extra physical memory allocated from round_page(_end[]) */
long kernend_extra;

/* dump configuration */
int	cpu_dump(void);
int	cpu_dumpsize(void);
u_long	cpu_dump_mempagecnt(void);

uint32_t dumpmag = 0x8fca0101;  /* magic number for savecore */
int     dumpsize = 0;           /* also for savecore */
long    dumplo = 0;

int aarch64_bti_enabled __read_mostly;

static void
bti_init(void)
{
#ifdef ARMV85_BTI
	extern uint64_t pmap_attr_gp;
	uint64_t reg;

	reg = reg_id_aa64pfr1_el1_read();

	if (reg >= ID_AA64PFR1_EL1_BT_SUPPORTED) {
		pmap_attr_gp = LX_BLKPAG_GP;
		aarch64_bti_enabled = 1;
	}
#endif
}

void
cpu_kernel_vm_init(uint64_t memory_start __unused, uint64_t memory_size __unused)
{
	extern char __kernel_text[];
	extern char _end[];
	extern char __data_start[];
	extern char __rodata_start[];
	u_int blk;

	bti_init();

	vaddr_t kernstart = trunc_page((vaddr_t)__kernel_text);
	vaddr_t kernend = round_page((vaddr_t)_end);
	paddr_t kernstart_phys = KERN_VTOPHYS(kernstart);
	paddr_t kernend_phys = KERN_VTOPHYS(kernend);
	vaddr_t data_start = (vaddr_t)__data_start;
	vaddr_t rodata_start = (vaddr_t)__rodata_start;

	/* add direct mappings of whole memory */
	const pt_entry_t dmattr =
	    LX_BLKPAG_ATTR_NORMAL_WB |
	    LX_BLKPAG_AP_RW |
	    LX_BLKPAG_PXN |
	    LX_BLKPAG_UXN;
	for (blk = 0; blk < bootconfig.dramblocks; blk++) {
		uint64_t start, end;

		start = trunc_page(bootconfig.dram[blk].address);
		end = round_page(bootconfig.dram[blk].address +
		    (uint64_t)bootconfig.dram[blk].pages * PAGE_SIZE);

		pmapboot_enter_range(AARCH64_PA_TO_KVA(start), start,
		    end - start, dmattr, printf);
	}

	/* Disable translation table walks using TTBR0 */
	uint64_t tcr = reg_tcr_el1_read();
	reg_tcr_el1_write(tcr | TCR_EPD0);
	isb();

	aarch64_tlbi_all();

	/*
	 * at this point, whole kernel image is mapped as "rwx".
	 * permission should be changed to:
	 *
	 *    text     rwx => r-x
	 *    rodata   rwx => r--
	 *    data     rwx => rw-  (.bss included)
	 *
	 * kernel image has mapped by L2 block. (2Mbyte)
	 */
	pmapboot_protect(L2_TRUNC_BLOCK(kernstart),
	    L2_TRUNC_BLOCK(data_start), VM_PROT_WRITE);
	pmapboot_protect(L2_ROUND_BLOCK(rodata_start),
	    L2_ROUND_BLOCK(kernend), VM_PROT_EXECUTE);

	aarch64_tlbi_all();

	VPRINTF("%s: kernel phys start %lx end %lx+%lx\n", __func__,
	    kernstart_phys, kernend_phys, kernend_extra);
	fdt_memory_remove_range(kernstart_phys,
	     kernend_phys - kernstart_phys + kernend_extra);

#ifdef KASAN
	kasan_kernelstart = kernstart;
	kasan_kernelsize = L2_ROUND_BLOCK(kernend) - kernstart;
#endif
}



/*
 * Upper region: 0xffff_ffff_ffff_ffff  Top of virtual memory
 *
 *               0xffff_ffff_ffe0_0000  End of KVA
 *                                      = VM_MAX_KERNEL_ADDRESS
 *
 *               0xffff_c000_4000_0000  Start of KVA
 *
 *               0xffff_c000_0???_????  End of kernel
 *                                      = _end[]
 *               0xffff_c000_00??_????  Start of kernel
 *                                      = __kernel_text[]
 *
 *               0xffff_c000_0000_0000  Kernel base address
 *                                      = VM_MIN_KERNEL_ADDRESS
 *
 *               0xffff_bfff_ffff_ffff  End of direct mapped
 *               0xffff_0000_0000_0000  Start of direct mapped
 *                                      = AARCH64_DIRECTMAP_START
 *
 * Hole:         0xfffe_ffff_ffff_ffff
 *               0x0001_0000_0000_0000
 *
 * Lower region: 0x0000_ffff_ffff_ffff  End of user address space
 *                                      = VM_MAXUSER_ADDRESS
 *
 *               0x0000_0000_0000_0000  Start of user address space
 */
vaddr_t
initarm_common(vaddr_t kvm_base, vsize_t kvm_size,
    const struct boot_physmem *bp, size_t nbp)
{
	extern char __kernel_text[];
	extern char _end[];
	extern char lwp0uspace[];

	struct pcb *pcb;
	struct trapframe *tf;
	psize_t memsize_total;
	vaddr_t kernstart, kernend;
	vaddr_t kernstart_l2 __unused, kernend_l2;	/* L2 table 2MB aligned */
	vaddr_t kernelvmstart;
	size_t i;

	cputype = cpu_idnum();	/* for compatible arm */

	kernstart = trunc_page((vaddr_t)__kernel_text);
	kernend = round_page((vaddr_t)_end);

	kernstart_l2 = L2_TRUNC_BLOCK(kernstart);
	kernend_l2 = L2_ROUND_BLOCK(kernend);

	kernelvmstart = kernend_l2;

#ifdef MODULAR
	/*
	 * The aarch64 compilers (gcc & llvm) use R_AARCH_CALL26/R_AARCH_JUMP26
	 * for function calls (bl)/jumps(b). At this time, neither compiler
	 * supports -mlong-calls therefore the kernel modules should be loaded
	 * within the maximum range of +/-128MB from kernel text.
	 */
#define MODULE_RESERVED_MAX	(1024 * 1024 * 128)
#define MODULE_RESERVED_SIZE	(1024 * 1024 * 32)	/* good enough? */
	module_start = kernelvmstart;
	module_end = kernend_l2 + MODULE_RESERVED_SIZE;
	if (module_end >= kernstart_l2 + MODULE_RESERVED_MAX)
		module_end = kernstart_l2 + MODULE_RESERVED_MAX;
	KASSERT(module_end > kernend_l2);
	kernelvmstart = module_end;
#endif /* MODULAR */

	KASSERT(kernelvmstart < VM_KERNEL_VM_BASE);

	kernelvmstart = VM_KERNEL_VM_BASE;

	paddr_t kernstart_phys __unused = KERN_VTOPHYS(kernstart);
	paddr_t kernend_phys __unused = KERN_VTOPHYS(kernend);

	/* XXX: arm/arm32/bus_dma.c refers physical_{start,end} */
	physical_start = bootconfig.dram[0].address;
	physical_end = bootconfig.dram[bootconfig.dramblocks - 1].address +
		       ptoa(bootconfig.dram[bootconfig.dramblocks - 1].pages);

	/*
	 * msgbuf is allocated from the bottom of any one of memory blocks
	 * to avoid corruption due to bootloader or changing kernel layout.
	 */
	paddr_t msgbufaddr = 0;
	for (i = 0; i < bootconfig.dramblocks; i++) {
		/* this block has enough space for msgbuf? */
		if (bootconfig.dram[i].pages < atop(round_page(MSGBUFSIZE)))
			continue;

		/* allocate msgbuf from the bottom of this block */
		bootconfig.dram[i].pages -= atop(round_page(MSGBUFSIZE));
		msgbufaddr = bootconfig.dram[i].address +
		    ptoa(bootconfig.dram[i].pages);
		break;
	}
	KASSERT(msgbufaddr != 0);	/* no space for msgbuf */
	initmsgbuf((void *)AARCH64_PA_TO_KVA(msgbufaddr), MSGBUFSIZE);

	VPRINTF(
	    "------------------------------------------\n"
	    "kern_vtopdiff         = 0x%016lx\n"
	    "physical_start        = 0x%016lx\n"
	    "kernel_start_phys     = 0x%016lx\n"
	    "kernel_end_phys       = 0x%016lx\n"
	    "pagetables_start_phys = 0x%016lx\n"
	    "pagetables_end_phys   = 0x%016lx\n"
	    "msgbuf                = 0x%016lx\n"
	    "physical_end          = 0x%016lx\n"
	    "VM_MIN_KERNEL_ADDRESS = 0x%016lx\n"
	    "kernel_start_l2       = 0x%016lx\n"
	    "kernel_start          = 0x%016lx\n"
	    "kernel_end            = 0x%016lx\n"
	    "(extra)               = 0x%016lx\n"
	    "kernel_end_l2         = 0x%016lx\n"
#ifdef MODULAR
	    "module_start          = 0x%016lx\n"
	    "module_end            = 0x%016lx\n"
#endif
	    "(kernel va area)      = 0x%016lx\n"
	    "(devmap va area)      = 0x%016lx\n"
	    "VM_MAX_KERNEL_ADDRESS = 0x%016lx\n"
	    "------------------------------------------\n",
	    kern_vtopdiff,
	    physical_start,
	    kernstart_phys,
	    kernend_phys,
	    round_page(kernend_phys),
	    round_page(kernend_phys) + kernend_extra,
	    msgbufaddr,
	    physical_end,
	    VM_MIN_KERNEL_ADDRESS,
	    kernstart_l2,
	    kernstart,
	    kernend,
	    kernend_extra,
	    kernend_l2,
#ifdef MODULAR
	    module_start,
	    module_end,
#endif
	    VM_KERNEL_VM_BASE,
	    VM_KERNEL_IO_ADDRESS,
	    VM_MAX_KERNEL_ADDRESS);

#ifdef DDB
	db_machdep_cpu_init();
#endif

	uvm_md_init();

	/* register free physical memory blocks */
	memsize_total = 0;

	KASSERT(bp != NULL || nbp == 0);
	KASSERT(bp == NULL || nbp != 0);

	KDASSERT(bootconfig.dramblocks <= DRAM_BLOCKS);
	for (i = 0; i < bootconfig.dramblocks; i++) {
		paddr_t start, end;

		/* empty is end */
		if (bootconfig.dram[i].address == 0 &&
		    bootconfig.dram[i].pages == 0)
			break;

		start = atop(bootconfig.dram[i].address);
		end = start + bootconfig.dram[i].pages;

		int vm_freelist = VM_FREELIST_DEFAULT;

		VPRINTF("block %2zu start %08lx  end %08lx\n", i, ptoa(start),
		    ptoa(end));

		/*
		 * This assumes the bp list is sorted in ascending
		 * order.
		 */
		paddr_t segend = end;
		for (size_t j = 0; j < nbp && start < end; j++) {
			paddr_t bp_start = bp[j].bp_start;
			paddr_t bp_end = bp_start + bp[j].bp_pages;

			VPRINTF("   bp %2zu start %08lx  end %08lx\n",
			    j, ptoa(bp_start), ptoa(bp_end));

			KASSERT(bp_start < bp_end);
			if (start >= bp_end || segend < bp_start)
				continue;

			if (start < bp_start)
				start = bp_start;

			if (start < bp_end) {
				if (segend > bp_end) {
					segend = bp_end;
				}
				vm_freelist = bp[j].bp_freelist;

				VPRINTF("         start %08lx  end %08lx"
				    "... loading in freelist %d\n", ptoa(start),
				    ptoa(segend), vm_freelist);

				uvm_page_physload(start, segend, start, segend,
				    vm_freelist);

				memsize_total += ptoa(segend - start);
				start = segend;
				segend = end;
			}
		}
	}
	physmem = atop(memsize_total);

	/*
	 * kernel image is mapped on L2 table (2MB*n) by locore.S
	 * virtual space start from 2MB aligned kernend
	 */
	pmap_bootstrap(kernelvmstart, VM_MAX_KERNEL_ADDRESS);

	kasan_init();

	/*
	 * setup lwp0
	 */
	uvm_lwp_setuarea(&lwp0, (vaddr_t)lwp0uspace);
	memset(&lwp0.l_md, 0, sizeof(lwp0.l_md));
	pcb = lwp_getpcb(&lwp0);
	memset(pcb, 0, sizeof(struct pcb));

	tf = (struct trapframe *)(lwp0uspace + USPACE) - 1;
	memset(tf, 0, sizeof(struct trapframe));
	tf->tf_spsr = SPSR_M_EL0T;
	lwp0.l_md.md_utf = pcb->pcb_tf = tf;

	return (vaddr_t)tf;
}

/*
 * machine dependent system variables.
 */
static xcfunc_t
set_user_tagged_address(void *arg1, void *arg2)
{
	uint64_t enable = PTRTOUINT64(arg1);
	uint64_t tcr = reg_tcr_el1_read();
	if (enable)
		tcr |= TCR_TBI0;
	else
		tcr &= ~TCR_TBI0;
	reg_tcr_el1_write(tcr);

	return 0;
}

static int
sysctl_machdep_tagged_address(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, cur, val;
	uint64_t tcr;

	tcr = reg_tcr_el1_read();
	cur = val = (tcr & TCR_TBI0) ? 1 : 0;

	node = *rnode;
	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (val < 0 || val > 1)
		return EINVAL;

	if (cur != val) {
		uint64_t where = xc_broadcast(0,
		    (xcfunc_t)set_user_tagged_address, UINT64TOPTR(val), NULL);
		xc_wait(where);
	}

	return 0;
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{
	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "tagged_address",
	    SYSCTL_DESCR("top byte ignored in the address calculation"),
	    sysctl_machdep_tagged_address, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_INT, "pan",
	    SYSCTL_DESCR("Whether Privileged Access Never is enabled"),
	    NULL, 0,
	    &aarch64_pan_enabled, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_INT, "pac",
	    SYSCTL_DESCR("Whether Pointer Authentication is enabled"),
	    NULL, 0,
	    &aarch64_pac_enabled, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_INT, "bti",
	    SYSCTL_DESCR("Whether Branch Target Identification is enabled"),
	    NULL, 0,
	    &aarch64_bti_enabled, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_INT, "hafdbs",
	    SYSCTL_DESCR("Whether Hardware updates to Access flag and Dirty state is enabled"),
	    NULL, 0,
	    &aarch64_hafdbs_enabled, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}

void
parse_mi_bootargs(char *args)
{
	const char *p = args;

	while (*p != '\0') {
		while (isspace(*p))
			p++;

		/* parse single dash (`-') options */
		if (*p == '-') {
			p++;
			while (!isspace(*p) && *p != '\0') {
				BOOT_FLAG(*p, boothowto);
				p++;
			}
			continue;
		}

		/* skip normal argument */
		while (!isspace(*p) && *p != '\0')
			p++;
	}
}

void
machdep_init(void)
{
	/* clear cpu reset hook for early boot */
	cpu_reset_address0 = NULL;

	configure_cpu_traps();
}

#ifdef MODULAR
/* Push any modules loaded by the boot loader */
void
module_init_md(void)
{
#ifdef FDT
	arm_fdt_module_init();
#endif
}
#endif /* MODULAR */

static bool
in_dram_p(paddr_t pa, psize_t size)
{
	int i;

	for (i = 0; i < bootconfig.dramblocks; i++) {
		paddr_t s, e;
		s = bootconfig.dram[i].address;
		e = bootconfig.dram[i].address + ptoa(bootconfig.dram[i].pages);
		if ((s <= pa) && ((pa + size) <= e))
			return true;
	}
	return false;
}

bool
mm_md_direct_mapped_phys(paddr_t pa, vaddr_t *vap)
{
	if (in_dram_p(pa, 0)) {
		*vap = AARCH64_PA_TO_KVA(pa);
		return true;
	}
	return false;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{
	if (in_dram_p(pa, 0))
		return 0;

	return kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL);
}

#ifdef __HAVE_MM_MD_KERNACC
int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{
	extern char __kernel_text[];
	extern char _end[];
	extern char __data_start[];
	extern char __rodata_start[];

	vaddr_t kernstart = trunc_page((vaddr_t)__kernel_text);
	vaddr_t kernend = round_page((vaddr_t)_end);
	paddr_t kernstart_phys = KERN_VTOPHYS(kernstart);
	vaddr_t data_start = (vaddr_t)__data_start;
	vaddr_t rodata_start = (vaddr_t)__rodata_start;
	vsize_t rosize = kernend - rodata_start;

	const vaddr_t v = (vaddr_t)ptr;

#define IN_RANGE(addr,sta,end)	(((sta) <= (addr)) && ((addr) < (end)))

	*handled = false;
	if (IN_RANGE(v, kernstart, kernend)) {
		*handled = true;
		if ((v < data_start) && (prot & VM_PROT_WRITE))
			return EFAULT;
	} else if (IN_RANGE(v, AARCH64_DIRECTMAP_START, AARCH64_DIRECTMAP_END)) {
		/*
		 * if defined PMAP_MAP_POOLPAGE, direct mapped address
		 * will be appeared as kvm(3) address.
		 */
		paddr_t pa = AARCH64_KVA_TO_PA(v);
		if (in_dram_p(pa, 0)) {
			*handled = true;
			if (IN_RANGE(pa, kernstart_phys,
			    kernstart_phys + rosize) &&
			    (prot & VM_PROT_WRITE))
				return EFAULT;
		}
	}
	return 0;
}
#endif

void
cpu_startup(void)
{
	vaddr_t maxaddr, minaddr;

	consinit();

#ifdef FDT
	const struct arm_platform * const plat = arm_fdt_platform();
	if (plat->ap_startup != NULL)
		plat->ap_startup();
#endif

	/*
	 * Allocate a submap for physio.
	 */
	minaddr = 0;
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	   VM_PHYS_SIZE, 0, FALSE, NULL);

#ifdef MODULAR
	uvm_map_setup(&module_map_store, module_start, module_end, 0);
	module_map_store.pmap = pmap_kernel();
	module_map = &module_map_store;
#endif

	/* Hello! */
	banner();

	cpu_startup_hook();
}

__weak_alias(cpu_startup_hook,cpu_startup_default)
void
cpu_startup_default(void)
{
}

/*
 * cpu_dump: dump the machine-dependent kernel core dump headers.
 */
int
cpu_dump(void)
{
	int (*dump)(dev_t, daddr_t, void *, size_t);
	char bf[dbtob(1)];
	kcore_seg_t *segp;
	cpu_kcore_hdr_t *cpuhdrp;
	phys_ram_seg_t *memsegp;
	const struct bdevsw *bdev;
	int i;

	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return (ENXIO);
	dump = bdev->d_dump;

	memset(bf, 0, sizeof bf);
	segp = (kcore_seg_t *)bf;
	cpuhdrp = (cpu_kcore_hdr_t *)&bf[ALIGN(sizeof(*segp))];
	memsegp = &cpuhdrp->kh_ramsegs[0];

	/*
	 * Generate a segment header.
	 */
	CORE_SETMAGIC(*segp, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	segp->c_size = dbtob(1) - ALIGN(sizeof(*segp));

	/*
	 * Add the machine-dependent header info.
	 */
	cpuhdrp->kh_tcr1 = reg_tcr_el1_read();
	cpuhdrp->kh_ttbr1 = reg_ttbr1_el1_read();
	cpuhdrp->kh_nramsegs = bootconfig.dramblocks;

	/*
	 * Fill in the memory segment descriptors.
	 */
	for (i = 0; i < bootconfig.dramblocks; i++) {
		memsegp[i].start = bootconfig.dram[i].address;
		memsegp[i].size = ptoa(bootconfig.dram[i].pages);
	}

	return (dump(dumpdev, dumplo, bf, dbtob(1)));
}

void
dumpsys(void)
{
	const struct bdevsw *bdev;
	daddr_t blkno;
	int psize;
	int error;
	paddr_t addr = 0, end;
	int block;
	psize_t len;
	vaddr_t dumpspace;

	/* flush everything out of caches */
	cpu_dcache_wbinv_all();

	if (dumpdev == NODEV)
		return;
	if (dumpsize == 0) {
		cpu_dumpconf();
	}
	if (dumplo <= 0 || dumpsize == 0) {
		printf("\ndump to dev %u,%u not possible\n",
		    major(dumpdev), minor(dumpdev));
		delay(5000000);
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n",
	    major(dumpdev), minor(dumpdev), dumplo);


	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL)
		return;
	psize = bdev_size(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	if ((error = cpu_dump()) != 0)
		goto err;

	blkno = dumplo + cpu_dumpsize();
	error = 0;
	len = dumpsize;

	for (block = 0; block < bootconfig.dramblocks && error == 0; ++block) {
		addr = bootconfig.dram[block].address;
		end = bootconfig.dram[block].address +
		      ptoa(bootconfig.dram[block].pages);
		for (; addr < end; addr += PAGE_SIZE) {
		    	if (((len * PAGE_SIZE) % (1024*1024)) == 0)
		    		printf("%lu ", (len * PAGE_SIZE) / (1024 * 1024));

			if (!mm_md_direct_mapped_phys(addr, &dumpspace)) {
				error = ENOMEM;
				goto err;
			}
			error = (*bdev->d_dump)(dumpdev,
			    blkno, (void *) dumpspace, PAGE_SIZE);

			if (error)
				goto err;
			blkno += btodb(PAGE_SIZE);
			len--;
		}
	}
err:
	switch (error) {
	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	case EINTR:
		printf("aborted from console\n");
		break;

	case ENOMEM:
		printf("no direct map for %lx\n", addr);
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
	printf("\n\n");
	delay(5000000);
}

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
int
cpu_dumpsize(void)
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)) +
	    ALIGN(bootconfig.dramblocks * sizeof(phys_ram_seg_t));
	if (roundup(size, dbtob(1)) != dbtob(1))
		return -1;

	return (1);
}

/*
 * cpu_dump_mempagecnt: calculate the size of RAM (in pages) to be dumped.
 */
u_long
cpu_dump_mempagecnt(void)
{
	u_long i, n;

	n = 0;
	for (i = 0; i < bootconfig.dramblocks; i++) {
		n += bootconfig.dram[i].pages;
	}

	return (n);
}

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first PAGE_SIZE of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */

void
cpu_dumpconf(void)
{
	u_long nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		return;
	nblks = bdev_size(dumpdev);
	if (nblks <= ctod(1))
		return;

	dumpblks = cpu_dumpsize();
	if (dumpblks < 0)
		goto bad;
	dumpblks += ctod(cpu_dump_mempagecnt());

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1)))
		goto bad;

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = cpu_dump_mempagecnt();
	return;

 bad:
	dumpsize = 0;
}
