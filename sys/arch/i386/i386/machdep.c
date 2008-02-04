/*	$NetBSD: machdep.c,v 1.563.2.9 2008/02/04 09:22:07 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000, 2004, 2006, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center and by Julio M. Merino Vidal.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.563.2.9 2008/02/04 09:22:07 yamt Exp $");

#include "opt_beep.h"
#include "opt_compat_ibcs2.h"
#include "opt_compat_mach.h"	/* need to get the right segment def */
#include "opt_compat_netbsd.h"
#include "opt_compat_svr4.h"
#include "opt_cpureset_delay.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_kgdb.h"
#include "opt_mtrr.h"
#include "opt_multiprocessor.h"
#include "opt_physmem.h"
#include "opt_realmem.h"
#include "opt_user_ldt.h"
#include "opt_vm86.h"
#include "opt_xbox.h"
#include "opt_xen.h"
#include "isa.h"
#include "pci.h"


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/extent.h>
#include <sys/syscallargs.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ucontext.h>
#include <sys/ras.h>
#include <sys/ksyms.h>

#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <dev/cons.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <sys/sysctl.h>

#include <x86/cpu_msr.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/gdt.h>
#include <machine/intr.h>
#include <machine/kcore.h>
#include <machine/pio.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/bootinfo.h>
#include <machine/mtrr.h>
#include <x86/x86/tsc.h>

#include <machine/multiboot.h>
#ifdef XEN
#include <xen/evtchn.h>
#include <xen/xen.h>
#include <xen/hypervisor.h>

/* #define	XENDEBUG */
/* #define	XENDEBUG_LOW */

#ifdef XENDEBUG
#define	XENPRINTF(x) printf x
#define	XENPRINTK(x) printk x
#else
#define	XENPRINTF(x)
#define	XENPRINTK(x)
#endif
#define	PRINTK(x) printf x
#endif /* XEN */

#include <dev/isa/isareg.h>
#include <machine/isa_machdep.h>
#include <dev/ic/i8042reg.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef VM86
#include <machine/vm86.h>
#endif

#ifdef XBOX
#include <machine/xbox.h>

int arch_i386_is_xbox = 0;
uint32_t arch_i386_xbox_memsize = 0;
#endif

#include "acpi.h"
#include "apmbios.h"
#include "bioscall.h"

#if NBIOSCALL > 0
#include <machine/bioscall.h>
#endif

#if NACPI > 0
#include <dev/acpi/acpivar.h>
#define ACPI_MACHDEP_PRIVATE
#include <machine/acpi_machdep.h>
#endif

#if NAPMBIOS > 0
#include <machine/apmvar.h>
#endif

#include "isa.h"
#include "isadma.h"
#include "npx.h"
#include "ksyms.h"

#include "cardbus.h"
#if NCARDBUS > 0
/* For rbus_min_start hint. */
#include <machine/bus.h>
#include <dev/cardbus/rbus.h>
#include <machine/rbus_machdep.h>
#endif

#include "mca.h"
#if NMCA > 0
#include <machine/mca_machdep.h>	/* for mca_busprobe() */
#endif

#ifdef MULTIPROCESSOR		/* XXX */
#include <machine/mpbiosvar.h>	/* XXX */
#endif				/* XXX */

/* the following is used externally (sysctl_hw) */
char machine[] = "i386";		/* CPU "architecture" */
char machine_arch[] = "i386";		/* machine == machine_arch */

extern struct bi_devmatch *x86_alldisks;
extern int x86_ndisks;

#ifdef CPURESET_DELAY
int	cpureset_delay = CPURESET_DELAY;
#else
int     cpureset_delay = 2000; /* default to 2s */
#endif

#ifdef MTRR
struct mtrr_funcs *mtrr_funcs;
#endif

#ifdef COMPAT_NOMID
static int exec_nomid(struct lwp *, struct exec_package *);
#endif

int	physmem;

unsigned int cpu_feature;
unsigned int cpu_feature2;
unsigned int cpu_feature_padlock;
int	cpu_class;
int	i386_fpu_present;
int	i386_fpu_exception;
int	i386_fpu_fdivbug;

int	i386_use_fxsave;
int	i386_has_sse;
int	i386_has_sse2;

int	tmx86_has_longrun;

vaddr_t	msgbuf_vaddr;
struct {
	paddr_t paddr;
	psize_t sz;
} msgbuf_p_seg[VM_PHYSSEG_MAX];
unsigned int msgbuf_p_cnt = 0;

vaddr_t	idt_vaddr;
paddr_t	idt_paddr;
vaddr_t	pentium_idt_vaddr;

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

extern	paddr_t avail_start, avail_end;
#ifdef XEN
extern paddr_t pmap_pa_start, pmap_pa_end;
void hypervisor_callback(void);
void failsafe_callback(void);
#endif

#ifdef XEN
void (*delay_func)(unsigned int) = xen_delay;
void (*initclock_func)(void) = xen_initclocks;
#else
void (*delay_func)(unsigned int) = i8254_delay;
void (*initclock_func)(void) = i8254_initclocks;
#endif


/*
 * Size of memory segments, before any memory is stolen.
 */
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int	mem_cluster_cnt;

void	init386(paddr_t);
void	initgdt(union descriptor *);

void	add_mem_cluster(uint64_t, uint64_t, uint32_t);

extern int time_adjusted;

struct bootinfo	bootinfo;
int *esym;
extern int boothowto;

#ifndef XEN

/* Base memory reported by BIOS. */
#ifndef REALBASEMEM
int	biosbasemem = 0;
#else
int	biosbasemem = REALBASEMEM;
#endif

/* Extended memory reported by BIOS. */
#ifndef REALEXTMEM
int	biosextmem = 0;
#else
int	biosextmem = REALEXTMEM;
#endif

/* Set if any boot-loader set biosbasemem/biosextmem. */
int	biosmem_implicit;

/* Representation of the bootinfo structure constructed by a NetBSD native
 * boot loader.  Only be used by native_loader(). */
struct bootinfo_source {
	uint32_t bs_naddrs;
	paddr_t bs_addrs[1]; /* Actually longer. */
};

/* Only called by locore.h; no need to be in a header file. */
void	native_loader(int, int, struct bootinfo_source *, paddr_t, int, int);

/*
 * Called as one of the very first things during system startup (just after
 * the boot loader gave control to the kernel image), this routine is in
 * charge of retrieving the parameters passed in by the boot loader and
 * storing them in the appropriate kernel variables.
 *
 * WARNING: Because the kernel has not yet relocated itself to KERNBASE,
 * special care has to be taken when accessing memory because absolute
 * addresses (referring to kernel symbols) do not work.  So:
 *
 *     1) Avoid jumps to absolute addresses (such as gotos and switches).
 *     2) To access global variables use their physical address, which
 *        can be obtained using the RELOC macro.
 */
void
native_loader(int bl_boothowto, int bl_bootdev,
    struct bootinfo_source *bl_bootinfo, paddr_t bl_esym,
    int bl_biosextmem, int bl_biosbasemem)
{
#define RELOC(type, x) ((type)((vaddr_t)(x) - KERNBASE))

	*RELOC(int *, &boothowto) = bl_boothowto;

#ifdef COMPAT_OLDBOOT
	/*
	 * Pre-1.3 boot loaders gave the boot device as a parameter
	 * (instead of a bootinfo entry).
	 */
	*RELOC(int *, &bootdev) = bl_bootdev;
#endif

	/*
	 * The boot loader provides a physical, non-relocated address
	 * for the symbols table's end.  We need to convert it to a
	 * virtual address.
	 */
	if (bl_esym != 0)
		*RELOC(int **, &esym) = (int *)((vaddr_t)bl_esym + KERNBASE);
	else
		*RELOC(int **, &esym) = 0;

	/*
	 * Copy bootinfo entries (if any) from the boot loader's
	 * representation to the kernel's bootinfo space.
	 */
	if (bl_bootinfo != NULL) {
		size_t i;
		uint8_t *data;
		struct bootinfo *bidest;

		bidest = RELOC(struct bootinfo *, &bootinfo);

		data = &bidest->bi_data[0];

		for (i = 0; i < bl_bootinfo->bs_naddrs; i++) {
			struct btinfo_common *bc;

			bc = (struct btinfo_common *)(bl_bootinfo->bs_addrs[i]);

			if ((paddr_t)(data + bc->len) >
			    (paddr_t)(&bidest->bi_data[0] + BOOTINFO_MAXSIZE))
				break;

			memcpy(data, bc, bc->len);
			data += bc->len;
		}
		bidest->bi_nentries = i;
	}

	/*
	 * Configure biosbasemem and biosextmem only if they were not
	 * explicitly given during the kernel's build.
	 */
	if (*RELOC(int *, &biosbasemem) == 0) {
		*RELOC(int *, &biosbasemem) = bl_biosbasemem;
		*RELOC(int *, &biosmem_implicit) = 1;
	}
	if (*RELOC(int *, &biosextmem) == 0) {
		*RELOC(int *, &biosextmem) = bl_biosextmem;
		*RELOC(int *, &biosmem_implicit) = 1;
	}
#undef RELOC
}

#endif /* XEN */

/*
 * Machine-dependent startup code
 */
void
cpu_startup()
{
	int x, y;
	vaddr_t minaddr, maxaddr;
	psize_t sz;
	char pbuf[9];

	/*
	 * For console drivers that require uvm and pmap to be initialized,
	 * we'll give them one more chance here...
	 */
	consinit();

#ifdef XBOX
	xbox_startup();
#endif

	/*
	 * Initialize error message buffer (et end of core).
	 */
	if (msgbuf_p_cnt == 0)
		panic("msgbuf paddr map has not been set up");
	for (x = 0, sz = 0; x < msgbuf_p_cnt; sz += msgbuf_p_seg[x++].sz)
		continue;
	msgbuf_vaddr = uvm_km_alloc(kernel_map, sz, 0, UVM_KMF_VAONLY);
	if (msgbuf_vaddr == 0)
		panic("failed to valloc msgbuf_vaddr");

	/* msgbuf_paddr was init'd in pmap */
	for (y = 0, sz = 0; y < msgbuf_p_cnt; y++) {
		for (x = 0; x < btoc(msgbuf_p_seg[y].sz); x++, sz += PAGE_SIZE)
			pmap_kenter_pa((vaddr_t)msgbuf_vaddr + sz,
			    msgbuf_p_seg[y].paddr + x * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE);
	}
	pmap_update(pmap_kernel());

	initmsgbuf((void *)msgbuf_vaddr, sz);

	printf("%s%s", copyright, version);

#ifdef MULTIBOOT
	multiboot_print_info();
#endif

#ifdef TRAPLOG
	/*
	 * Enable recording of branch from/to in MSR's
	 */
	wrmsr(MSR_DEBUGCTLMSR, 0x1);
#endif

	format_bytes(pbuf, sizeof(pbuf), ptoa(physmem));
	printf("total memory = %s\n", pbuf);

#if NCARDBUS > 0
	/* Tell RBUS how much RAM we have, so it can use heuristics. */
	rbus_min_start_hint(ptoa(physmem));
#endif

	minaddr = 0;

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16*NCARGS, VM_MAP_PAGEABLE, false, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, false, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/* Safe for i/o port / memory space allocation to use malloc now. */
#if !defined(XEN) || defined(DOM0OPS)
	x86_bus_space_mallocok();
#endif

	gdt_init();
	i386_proc0_tss_ldt_init();

#ifndef XEN
	cpu_init_tss(&cpu_info_primary);
	ltr(cpu_info_primary.ci_tss_sel);
#endif

	x86_init();
}

/*
 * Set up proc0's TSS and LDT.
 */
void
i386_proc0_tss_ldt_init()
{
	struct lwp *l;
	struct pcb *pcb;

	l = &lwp0;
	pcb = &l->l_addr->u_pcb;

	pcb->pcb_ldt_sel = pmap_kernel()->pm_ldt_sel = GSEL(GLDT_SEL, SEL_KPL);
	pcb->pcb_cr0 = rcr0();
	pcb->pcb_esp0 = USER_TO_UAREA(l->l_addr) + KSTACK_SIZE - 16;
	pcb->pcb_iopl = SEL_KPL;
	l->l_md.md_regs = (struct trapframe *)pcb->pcb_esp0 - 1;
	memcpy(pcb->pcb_fsd, &gdt[GUDATA_SEL], sizeof(pcb->pcb_fsd));
	memcpy(pcb->pcb_gsd, &gdt[GUDATA_SEL], sizeof(pcb->pcb_gsd));

#ifndef XEN
	lldt(pcb->pcb_ldt_sel);
#else
	HYPERVISOR_fpu_taskswitch();
	XENPRINTF(("lwp tss sp %p ss %04x/%04x\n",
	    (void *)pcb->pcb_esp0,
	    GSEL(GDATA_SEL, SEL_KPL),
	    IDXSEL(GSEL(GDATA_SEL, SEL_KPL))));
	HYPERVISOR_stack_switch(GSEL(GDATA_SEL, SEL_KPL), pcb->pcb_esp0);
#endif
}

#ifdef XEN
/*
 * Switch context:
 * - honor CR0_TS in saved CR0 and request DNA exception on FPU use
 * - switch stack pointer for user->kernel transition
 */
void
i386_switch_context(lwp_t *l)
{
	struct cpu_info *ci;
	struct pcb *pcb = &l->l_addr->u_pcb;

	ci = curcpu();
	if (ci->ci_fpused) {
		HYPERVISOR_fpu_taskswitch();
		ci->ci_fpused = 0;
	}

	HYPERVISOR_stack_switch(GSEL(GDATA_SEL, SEL_KPL), pcb->pcb_esp0);

	if (xen_start_info.flags & SIF_PRIVILEGED) {
		int iopl = pcb->pcb_iopl;
#ifdef XEN3
	        struct physdev_op physop;
		physop.cmd = PHYSDEVOP_SET_IOPL;
		physop.u.set_iopl.iopl = iopl;
		HYPERVISOR_physdev_op(&physop);
#else
		dom0_op_t op;
		op.cmd = DOM0_IOPL;
		op.u.iopl.domain = DOMID_SELF;
		op.u.iopl.iopl = iopl;
		HYPERVISOR_dom0_op(&op);
#endif
	}
}
#endif /* XEN */

#ifndef XEN
/*
 * Set up TSS and I/O bitmap.
 */
void
cpu_init_tss(struct cpu_info *ci)
{
	struct i386tss *tss = &ci->ci_tss;

	tss->tss_iobase = IOMAP_INVALOFF << 16;
	tss->tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	tss->tss_ldt = GSEL(GLDT_SEL, SEL_KPL);
	tss->tss_cr3 = rcr3();
	ci->ci_tss_sel = tss_alloc(tss);
}
#endif /* XEN */

/*
 * sysctl helper routine for machdep.tm* nodes.
 */
static int
sysctl_machdep_tm_longrun(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int io, error;

	if (!tmx86_has_longrun)
		return (EOPNOTSUPP);

	node = *rnode;
	node.sysctl_data = &io;

	switch (rnode->sysctl_num) {
	case CPU_TMLR_MODE:
		io = (int)(crusoe_longrun = tmx86_get_longrun_mode());
		break;
	case CPU_TMLR_FREQUENCY:
		tmx86_get_longrun_status_all();
		io = crusoe_frequency;
		break;
	case CPU_TMLR_VOLTAGE:
		tmx86_get_longrun_status_all();
		io = crusoe_voltage;
		break;
	case CPU_TMLR_PERCENTAGE:
		tmx86_get_longrun_status_all();
		io = crusoe_percentage;
		break;
	default:
		return (EOPNOTSUPP);
	}

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (rnode->sysctl_num == CPU_TMLR_MODE) {
		if (tmx86_set_longrun_mode(io))
			crusoe_longrun = (u_int)io;
		else
			return (EINVAL);
	}

	return (0);
}

/*
 * sysctl helper routine for machdep.booted_kernel
 */
static int
sysctl_machdep_booted_kernel(SYSCTLFN_ARGS)
{
	struct btinfo_bootpath *bibp;
	struct sysctlnode node;

	bibp = lookup_bootinfo(BTINFO_BOOTPATH);
	if(!bibp)
		return(ENOENT); /* ??? */

	node = *rnode;
	node.sysctl_data = bibp->bootpath;
	node.sysctl_size = sizeof(bibp->bootpath);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * sysctl helper routine for machdep.diskinfo
 */
static int
sysctl_machdep_diskinfo(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	node = *rnode;
	if (x86_alldisks == NULL)
		return(EOPNOTSUPP);
	node.sysctl_data = x86_alldisks;
	node.sysctl_size = sizeof(struct disklist) +
	    (x86_ndisks - 1) * sizeof(struct nativedisk_info);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

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
#ifndef XEN
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "biosbasemem", NULL,
		       NULL, 0, &biosbasemem, 0,
		       CTL_MACHDEP, CPU_BIOSBASEMEM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "biosextmem", NULL,
		       NULL, 0, &biosextmem, 0,
		       CTL_MACHDEP, CPU_BIOSEXTMEM, CTL_EOL);
#endif /* XEN */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_booted_kernel, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "diskinfo", NULL,
		       sysctl_machdep_diskinfo, 0, NULL, 0,
		       CTL_MACHDEP, CPU_DISKINFO, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "fpu_present", NULL,
		       NULL, 0, &i386_fpu_present, 0,
		       CTL_MACHDEP, CPU_FPU_PRESENT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "osfxsr", NULL,
		       NULL, 0, &i386_use_fxsave, 0,
		       CTL_MACHDEP, CPU_OSFXSR, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "sse", NULL,
		       NULL, 0, &i386_has_sse, 0,
		       CTL_MACHDEP, CPU_SSE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "sse2", NULL,
		       NULL, 0, &i386_has_sse2, 0,
		       CTL_MACHDEP, CPU_SSE2, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL, 
	    	       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "cpu_brand", NULL,
		       NULL, 0, &cpu_brand_string, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "tm_longrun_mode", NULL,
		       sysctl_machdep_tm_longrun, 0, NULL, 0,
		       CTL_MACHDEP, CPU_TMLR_MODE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "tm_longrun_frequency", NULL,
		       sysctl_machdep_tm_longrun, 0, NULL, 0,
		       CTL_MACHDEP, CPU_TMLR_FREQUENCY, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "tm_longrun_voltage", NULL,
		       sysctl_machdep_tm_longrun, 0, NULL, 0,
		       CTL_MACHDEP, CPU_TMLR_VOLTAGE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "tm_longrun_percentage", NULL,
		       sysctl_machdep_tm_longrun, 0, NULL, 0,
		       CTL_MACHDEP, CPU_TMLR_PERCENTAGE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "sparse_dump", NULL,
		       NULL, 0, &sparse_dump, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}

void *
getframe(struct lwp *l, int sig, int *onstack)
{
	struct proc *p = l->l_proc;
	struct trapframe *tf = l->l_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	*onstack = (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	if (*onstack)
		return (char *)l->l_sigstk.ss_sp + l->l_sigstk.ss_size;
#ifdef VM86
	if (tf->tf_eflags & PSL_VM)
		return (void *)(tf->tf_esp + (tf->tf_ss << 4));
	else
#endif
		return (void *)tf->tf_esp;
}

/*
 * Build context to run handler in.  We invoke the handler
 * directly, only returning via the trampoline.  Note the
 * trampoline version numbers are coordinated with machine-
 * dependent code in libc.
 */
void
buildcontext(struct lwp *l, int sel, void *catcher, void *fp)
{
	struct trapframe *tf = l->l_md.md_regs;

#ifndef XEN
	tf->tf_gs = GSEL(GUGS_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUFS_SEL, SEL_UPL);
#else
	tf->tf_gs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUDATA_SEL, SEL_UPL);
#endif
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_eip = (int)catcher;
	tf->tf_cs = GSEL(sel, SEL_UPL);
	tf->tf_eflags &= ~(PSL_T|PSL_VM|PSL_AC|PSL_D);
	tf->tf_esp = (int)fp;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);
}

static void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct pmap *pmap = vm_map_pmap(&p->p_vmspace->vm_map);
	int sel = pmap->pm_hiexec > I386_MAX_EXE_ADDR ?
	    GUCODEBIG_SEL : GUCODE_SEL;
	struct sigacts *ps = p->p_sigacts;
	int onstack, error;
	int sig = ksi->ksi_signo;
	struct sigframe_siginfo *fp = getframe(l, sig, &onstack), frame;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct trapframe *tf = l->l_md.md_regs;

	KASSERT(mutex_owned(&p->p_smutex));

	fp--;

	/* Build stack frame for signal trampoline. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	case 0:		/* handled by sendsig_sigcontext */
	case 1:		/* handled by sendsig_sigcontext */
	default:	/* unknown version */
		printf("nsendsig: bad version %d\n",
		    ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
	case 2:
		break;
	}

	frame.sf_ra = (int)ps->sa_sigdesc[sig].sd_tramp;
	frame.sf_signum = sig;
	frame.sf_sip = &fp->sf_si;
	frame.sf_ucp = &fp->sf_uc;
	frame.sf_si._info = ksi->ksi_info;
	frame.sf_uc.uc_flags = _UC_SIGMASK|_UC_VM;
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_link = l->l_ctxlink;
	frame.sf_uc.uc_flags |= (l->l_sigstk.ss_flags & SS_ONSTACK)
	    ? _UC_SETSTACK : _UC_CLRSTACK;
	memset(&frame.sf_uc.uc_stack, 0, sizeof(frame.sf_uc.uc_stack));

	if (tf->tf_eflags & PSL_VM)
		(*p->p_emul->e_syscall_intern)(p);
	sendsig_reset(l, sig);

	mutex_exit(&p->p_smutex);
	cpu_getmcontext(l, &frame.sf_uc.uc_mcontext, &frame.sf_uc.uc_flags);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(&p->p_smutex);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	buildcontext(l, sel, catcher, fp);

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{

	KASSERT(mutex_owned(&curproc->p_smutex));

#ifdef COMPAT_16
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		sendsig_sigcontext(ksi, mask);
	else
#endif
		sendsig_siginfo(ksi, mask);
}

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		if (time_adjusted != 0)
			resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

#ifdef MULTIPROCESSOR
	x86_broadcast_ipi(X86_IPI_HALT);
#endif

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
#ifdef XEN
		HYPERVISOR_shutdown();
		for (;;);
#endif
#ifdef XBOX
		if (arch_i386_is_xbox) {
			xbox_poweroff();
			for (;;);
		}
#endif
#if NACPI > 0
		if (acpi_softc != NULL) {
			delay(500000);
			acpi_enter_sleep_state(acpi_softc, ACPI_STATE_S5);
			printf("WARNING: ACPI powerdown failed!\n");
		}
#endif
#if NAPMBIOS > 0 && !defined(APM_NO_POWEROFF)
		/* turn off, if we can.  But try to turn disk off and
		 * wait a bit first--some disk drives are slow to clean up
		 * and users have reported disk corruption.
		 */
		delay(500000);
		apm_set_powstate(NULL, APM_DEV_DISK(APM_DEV_ALLUNITS), APM_SYS_OFF);
		delay(500000);
		apm_set_powstate(NULL, APM_DEV_ALLDEVS, APM_SYS_OFF);
		printf("WARNING: APM powerdown failed!\n");
		/*
		 * RB_POWERDOWN implies RB_HALT... fall into it...
		 */
#endif
	}

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");

#ifdef BEEP_ONHALT
		{
			int c;
			for (c = BEEP_ONHALT_COUNT; c > 0; c--) {
				sysbeep(BEEP_ONHALT_PITCH,
					BEEP_ONHALT_PERIOD * hz / 1000);
				delay(BEEP_ONHALT_PERIOD * 1000);
				sysbeep(0, BEEP_ONHALT_PERIOD * hz / 1000);
				delay(BEEP_ONHALT_PERIOD * 1000);
			}
		}
#endif

		cnpollc(1);	/* for proper keyboard command handling */
		if (cngetc() == 0) {
			/* no console attached, so just hlt */
			for(;;) {
				x86_hlt();
			}
		}
		cnpollc(0);
	}

	printf("rebooting...\n");
	if (cpureset_delay > 0)
		delay(cpureset_delay * 1000);
	cpu_reset();
	for(;;) ;
	/*NOTREACHED*/
}

/*
 * Clear registers on exec
 */
void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct pmap *pmap = vm_map_pmap(&l->l_proc->p_vmspace->vm_map);
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf;

#if NNPX > 0
	/* If we were using the FPU, forget about it. */
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		npxsave_lwp(l, false);
#endif

#ifdef USER_LDT
	pmap_ldt_cleanup(l);
#endif

	l->l_md.md_flags &= ~MDL_USEDFPU;
	if (i386_use_fxsave) {
		pcb->pcb_savefpu.sv_xmm.sv_env.en_cw = __NetBSD_NPXCW__;
		pcb->pcb_savefpu.sv_xmm.sv_env.en_mxcsr = __INITIAL_MXCSR__;
	} else
		pcb->pcb_savefpu.sv_87.sv_env.en_cw = __NetBSD_NPXCW__;
	memcpy(pcb->pcb_fsd, &gdt[GUDATA_SEL], sizeof(pcb->pcb_fsd));
	memcpy(pcb->pcb_gsd, &gdt[GUDATA_SEL], sizeof(pcb->pcb_gsd));

	tf = l->l_md.md_regs;
#ifndef XEN
	tf->tf_gs = GSEL(GUGS_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUFS_SEL, SEL_UPL);
#else
	tf->tf_gs = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_fs = LSEL(LUDATA_SEL, SEL_UPL);
#endif
	tf->tf_es = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_ds = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_edi = 0;
	tf->tf_esi = 0;
	tf->tf_ebp = 0;
	tf->tf_ebx = (int)l->l_proc->p_psstr;
	tf->tf_edx = 0;
	tf->tf_ecx = 0;
	tf->tf_eax = 0;
	tf->tf_eip = pack->ep_entry;
	tf->tf_cs = pmap->pm_hiexec > I386_MAX_EXE_ADDR ?
	    LSEL(LUCODEBIG_SEL, SEL_UPL) : LSEL(LUCODE_SEL, SEL_UPL);
	tf->tf_eflags = PSL_USERSET;
	tf->tf_esp = stack;
	tf->tf_ss = LSEL(LUDATA_SEL, SEL_UPL);
}

/*
 * Initialize segments and descriptor tables
 */

union	descriptor *gdt, *ldt;
union	descriptor *pentium_idt;
struct user *proc0paddr;
extern vaddr_t proc0uarea;

void
setgate(struct gate_descriptor *gd, void *func, int args, int type, int dpl,
    int sel)
{

	gd->gd_looffset = (int)func;
	gd->gd_selector = sel;
	gd->gd_stkcpy = args;
	gd->gd_xx = 0;
	gd->gd_type = type;
	gd->gd_dpl = dpl;
	gd->gd_p = 1;
	gd->gd_hioffset = (int)func >> 16;
}

void
unsetgate(struct gate_descriptor *gd)
{
	gd->gd_p = 0;
	gd->gd_hioffset = 0;
	gd->gd_looffset = 0;
	gd->gd_selector = 0;
	gd->gd_xx = 0;
	gd->gd_stkcpy = 0;
	gd->gd_type = 0;
	gd->gd_dpl = 0;
}


void
setregion(struct region_descriptor *rd, void *base, size_t limit)
{

	rd->rd_limit = (int)limit;
	rd->rd_base = (int)base;
}

void
setsegment(struct segment_descriptor *sd, const void *base, size_t limit,
    int type, int dpl, int def32, int gran)
{

	sd->sd_lolimit = (int)limit;
	sd->sd_lobase = (int)base;
	sd->sd_type = type;
	sd->sd_dpl = dpl;
	sd->sd_p = 1;
	sd->sd_hilimit = (int)limit >> 16;
	sd->sd_xx = 0;
	sd->sd_def32 = def32;
	sd->sd_gran = gran;
	sd->sd_hibase = (int)base >> 24;
}

#define	IDTVEC(name)	__CONCAT(X, name)
typedef void (vector)(void);
extern vector IDTVEC(syscall);
extern vector IDTVEC(osyscall);
extern vector *IDTVEC(exceptions)[];
#ifdef COMPAT_SVR4
extern vector IDTVEC(svr4_fasttrap);
#endif /* COMPAT_SVR4 */
#ifdef COMPAT_MACH
extern vector IDTVEC(mach_trap);
#endif
#ifdef XEN
#define MAX_XEN_IDT 128
trap_info_t xen_idt[MAX_XEN_IDT];
int xen_idt_idx;
#endif

#define	KBTOB(x)	((size_t)(x) * 1024UL)
#define	MBTOB(x)	((size_t)(x) * 1024UL * 1024UL)

#ifndef XEN
void cpu_init_idt()
{
	struct region_descriptor region;
	setregion(&region, pentium_idt, NIDT * sizeof(idt[0]) - 1);
	lidt(&region);
}

void
add_mem_cluster(uint64_t seg_start, uint64_t seg_end, uint32_t type)
{
	extern struct extent *iomem_ex;
	uint64_t new_physmem;
	int i;

	if (seg_end > 0x100000000ULL) {
		printf("WARNING: skipping large "
		    "memory map entry: "
		    "0x%qx/0x%qx/0x%x\n",
		    seg_start,
		    (seg_end - seg_start),
		    type);
		return;
	}

	/*
	 * XXX Chop the last page off the size so that
	 * XXX it can fit in avail_end.
	 */
	if (seg_end == 0x100000000ULL)
		seg_end -= PAGE_SIZE;

	if (seg_end <= seg_start)
		return;

	for (i = 0; i < mem_cluster_cnt; i++) {
		if ((mem_clusters[i].start == round_page(seg_start))
		    && (mem_clusters[i].size
			    == trunc_page(seg_end) - mem_clusters[i].start)) {
#ifdef DEBUG_MEMLOAD
			printf("WARNING: skipping duplicate segment entry\n");
#endif
			return;
		}
	}

	/*
	 * Allocate the physical addresses used by RAM
	 * from the iomem extent map.  This is done before
	 * the addresses are page rounded just to make
	 * sure we get them all.
	 */
	if (extent_alloc_region(iomem_ex, seg_start,
	    seg_end - seg_start, EX_NOWAIT)) {
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE "
		    "MEMORY SEGMENT "
		    "(0x%qx/0x%qx/0x%x) FROM "
		    "IOMEM EXTENT MAP!\n",
		    seg_start, seg_end - seg_start, type);
		return;
	}

	/*
	 * If it's not free memory, skip it.
	 */
	if (type != BIM_Memory)
		return;

	/* XXX XXX XXX */
	if (mem_cluster_cnt >= VM_PHYSSEG_MAX)
		panic("init386: too many memory segments "
		    "(increase VM_PHYSSEG_MAX)");

#ifdef PHYSMEM_MAX_ADDR
	if (seg_start >= MBTOB(PHYSMEM_MAX_ADDR))
		return;
	if (seg_end > MBTOB(PHYSMEM_MAX_ADDR))
		seg_end = MBTOB(PHYSMEM_MAX_ADDR);
#endif

	seg_start = round_page(seg_start);
	seg_end = trunc_page(seg_end);

	if (seg_start == seg_end)
		return;

	mem_clusters[mem_cluster_cnt].start = seg_start;
	new_physmem = physmem + atop(seg_end - seg_start);

#ifdef PHYSMEM_MAX_SIZE
	if (physmem >= atop(MBTOB(PHYSMEM_MAX_SIZE)))
		return;
	if (new_physmem > atop(MBTOB(PHYSMEM_MAX_SIZE))) {
		seg_end = seg_start + MBTOB(PHYSMEM_MAX_SIZE) - ptoa(physmem);
		new_physmem = atop(MBTOB(PHYSMEM_MAX_SIZE));
	}
#endif

	mem_clusters[mem_cluster_cnt].size = seg_end - seg_start;

	if (avail_end < seg_end)
		avail_end = seg_end;
	physmem = new_physmem;
	mem_cluster_cnt++;
}
#endif /* !XEN */

void
initgdt(union descriptor *tgdt)
{
#ifdef XEN
	u_long	frames[16];
#else
	struct region_descriptor region;
	gdt = tgdt;
	memset(gdt, 0, NGDT*sizeof(*gdt));
#endif /* XEN */
	/* make gdt gates and memory segments */
	setsegment(&gdt[GCODE_SEL].sd, 0, 0xfffff, SDT_MEMERA, SEL_KPL, 1, 1);
	setsegment(&gdt[GDATA_SEL].sd, 0, 0xfffff, SDT_MEMRWA, SEL_KPL, 1, 1);
	setsegment(&gdt[GUCODE_SEL].sd, 0, x86_btop(I386_MAX_EXE_ADDR) - 1,
	    SDT_MEMERA, SEL_UPL, 1, 1);
	setsegment(&gdt[GUCODEBIG_SEL].sd, 0, 0xfffff,
	    SDT_MEMERA, SEL_UPL, 1, 1);
	setsegment(&gdt[GUDATA_SEL].sd, 0, 0xfffff,
	    SDT_MEMRWA, SEL_UPL, 1, 1);
#ifdef COMPAT_MACH
	setgate(&gdt[GMACHCALLS_SEL].gd, &IDTVEC(mach_trap), 1,
	    SDT_SYS386CGT, SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));
#endif
#if NBIOSCALL > 0
	/* bios trampoline GDT entries */
	setsegment(&gdt[GBIOSCODE_SEL].sd, 0, 0xfffff, SDT_MEMERA, SEL_KPL, 0,
	    0);
	setsegment(&gdt[GBIOSDATA_SEL].sd, 0, 0xfffff, SDT_MEMRWA, SEL_KPL, 0,
	    0);
#endif
	setsegment(&gdt[GCPU_SEL].sd, &cpu_info_primary, 0xfffff,
	    SDT_MEMRWA, SEL_KPL, 1, 1);

#ifndef XEN
	setregion(&region, gdt, NGDT * sizeof(gdt[0]) - 1);
	lgdt(&region);
#else /* !XEN */
	frames[0] = xpmap_ptom((uint32_t)gdt - KERNBASE) >> PAGE_SHIFT;
	pmap_kenter_pa((vaddr_t)gdt, (uint32_t)gdt - KERNBASE, VM_PROT_READ);
#ifdef XEN3
	XENPRINTK(("loading gdt %lx, %d entries\n", frames[0] << PAGE_SHIFT,
	    NGDT));
	if (HYPERVISOR_set_gdt(frames, NGDT /* XXX is it right ? */))
		panic("HYPERVISOR_set_gdt failed!\n");
#else
	XENPRINTK(("loading gdt %lx, %d entries\n", frames[0] << PAGE_SHIFT,
	    LAST_RESERVED_GDT_ENTRY + 1));
	if (HYPERVISOR_set_gdt(frames, LAST_RESERVED_GDT_ENTRY + 1))
		panic("HYPERVISOR_set_gdt failed!\n");
#endif
	lgdt_finish();

#endif /* !XEN */
}

static void
init386_msgbuf(void)
{
	/* Message buffer is located at end of core. */
	struct vm_physseg *vps;
	psize_t sz = round_page(MSGBUFSIZE);
	psize_t reqsz = sz;
	unsigned int x;

 search_again:
	vps = NULL;
	for (x = 0; x < vm_nphysseg; ++x) {
		vps = &vm_physmem[x];
		if (ptoa(vps->avail_end) == avail_end) {
			break;
		}
	}
	if (x == vm_nphysseg)
		panic("init386: can't find end of memory");

	/* Shrink so it'll fit in the last segment. */
	if (vps->avail_end - vps->avail_start < atop(sz))
		sz = ptoa(vps->avail_end - vps->avail_start);

	vps->avail_end -= atop(sz);
	vps->end -= atop(sz);
	msgbuf_p_seg[msgbuf_p_cnt].sz = sz;
	msgbuf_p_seg[msgbuf_p_cnt++].paddr = ptoa(vps->avail_end);

	/* Remove the last segment if it now has no pages. */
	if (vps->start == vps->end) {
		for (--vm_nphysseg; x < vm_nphysseg; x++)
			vm_physmem[x] = vm_physmem[x + 1];
	}

	/* Now find where the new avail_end is. */
	for (avail_end = 0, x = 0; x < vm_nphysseg; x++)
		if (vm_physmem[x].avail_end > avail_end)
			avail_end = vm_physmem[x].avail_end;
	avail_end = ptoa(avail_end);

	if (sz == reqsz)
		return;

	reqsz -= sz;
	if (msgbuf_p_cnt == VM_PHYSSEG_MAX) {
		/* No more segments available, bail out. */
		printf("WARNING: MSGBUFSIZE (%zu) too large, using %zu.\n",
		    (size_t)MSGBUFSIZE, (size_t)(MSGBUFSIZE - reqsz));
		return;
	}

	sz = reqsz;
	goto search_again;
}

#ifndef XEN
static void
init386_pte0(void)
{
	paddr_t paddr;
	vaddr_t vaddr;

	paddr = 4 * PAGE_SIZE;
	vaddr = (vaddr_t)vtopte(0);
	pmap_kenter_pa(vaddr, paddr, VM_PROT_READ | VM_PROT_WRITE);
	pmap_update(pmap_kernel());
	/* make sure it is clean before using */
	memset((void *)vaddr, 0, PAGE_SIZE);
}
#endif /* !XEN */

static void
init386_ksyms(void)
{
#if NKSYMS || defined(DDB) || defined(LKM)
	extern int end;
	struct btinfo_symtab *symtab;

#ifdef DDB
	db_machine_init();
#endif

#if defined(MULTIBOOT)
	if (multiboot_ksyms_init())
		return;
#endif

	if ((symtab = lookup_bootinfo(BTINFO_SYMTAB)) == NULL) {
		ksyms_init(*(int *)&end, ((int *)&end) + 1, esym);
		return;
	}

	symtab->ssym += KERNBASE;
	symtab->esym += KERNBASE;
	ksyms_init(symtab->nsym, (int *)symtab->ssym, (int *)symtab->esym);
#endif
}

void
init386(paddr_t first_avail)
{
#ifndef XEN
	union descriptor *tgdt;
	extern struct extent *iomem_ex;
	struct btinfo_memmap *bim;
	struct region_descriptor region;
	int first16q;
	uint64_t seg_start, seg_end;
	uint64_t seg_start1, seg_end1;
#endif
	extern void consinit(void);
	int x;
#if NBIOSCALL > 0
	extern int biostramp_image_size;
	extern u_char biostramp_image[];
#endif


#ifdef XEN
	XENPRINTK(("HYPERVISOR_shared_info %p (%x)\n", HYPERVISOR_shared_info,
	    xen_start_info.shared_info));
#endif
	cpu_probe_features(&cpu_info_primary);
	cpu_feature = cpu_info_primary.ci_feature_flags;
	cpu_feature2 = cpu_info_primary.ci_feature2_flags;
	cpu_feature_padlock = cpu_info_primary.ci_padlock_flags;

	proc0paddr = UAREA_TO_USER(proc0uarea);
	lwp0.l_addr = proc0paddr;

#ifdef XEN
	/* not on Xen... */
	cpu_feature &= ~(CPUID_PGE|CPUID_PSE|CPUID_MTRR|CPUID_FXSR|CPUID_NOX);
	lwp0.l_addr->u_pcb.pcb_cr3 = PDPpaddr - KERNBASE;
	__PRINTK(("pcb_cr3 0x%lx cr3 0x%lx\n",
	    PDPpaddr - KERNBASE, xpmap_ptom(PDPpaddr - KERNBASE)));
	XENPRINTK(("proc0paddr %p first_avail %p\n",
	    proc0paddr, (void *)(long)first_avail));
	XENPRINTK(("ptdpaddr %p atdevbase %p\n", (void *)PDPpaddr,
	    (void *)atdevbase));
#endif


#ifdef XBOX
	/*
	 * From Rink Springer @ FreeBSD:
	 *
	 * The following code queries the PCI ID of 0:0:0. For the XBOX,
	 * This should be 0x10de / 0x02a5.
	 *
	 * This is exactly what Linux does.
	 */
	outl(0xcf8, 0x80000000);
	if (inl(0xcfc) == 0x02a510de) {
		arch_i386_is_xbox = 1;
		xbox_lcd_init();
		xbox_lcd_writetext("NetBSD/i386 ");

		/*
		 * We are an XBOX, but we may have either 64MB or 128MB of
		 * memory. The PCI host bridge should be programmed for this,
		 * so we just query it. 
		 */
		outl(0xcf8, 0x80000084);
		arch_i386_xbox_memsize = (inl(0xcfc) == 0x7FFFFFF) ? 128 : 64;
	}
#endif /* XBOX */

#if NISA > 0 || NPCI > 0
	x86_bus_space_init();
#endif
#ifdef XEN
	xen_parse_cmdline(XEN_PARSE_BOOTFLAGS, NULL);
#endif

	/*
	 * Initailize PAGE_SIZE-dependent variables.
	 */
	uvm_setpagesize();

	/*
	 * Saving SSE registers won't work if the save area isn't
	 * 16-byte aligned.
	 */
	if (offsetof(struct user, u_pcb.pcb_savefpu) & 0xf)
		panic("init386: pcb_savefpu not 16-byte aligned");

	/*
	 * Start with 2 color bins -- this is just a guess to get us
	 * started.  We'll recolor when we determine the largest cache
	 * sizes on the system.
	 */
	uvmexp.ncolors = 2;

#ifndef XEN
	/*
	 * Low memory reservations:
	 * Page 0:	BIOS data
	 * Page 1:	BIOS callback
	 * Page 2:	MP bootstrap
	 * Page 3:	ACPI wakeup code
	 * Page 4:	Temporary page table for 0MB-4MB
	 * Page 5:	Temporary page directory
	 */
	avail_start = 6 * PAGE_SIZE;
#else /* !XEN */
	/* steal one page for gdt */
	gdt = (void *)((u_long)first_avail + KERNBASE);
	first_avail += PAGE_SIZE;
	/* Make sure the end of the space used by the kernel is rounded. */
	first_avail = round_page(first_avail);
	avail_start = first_avail;
	avail_end = ptoa(xen_start_info.nr_pages) + XPMAP_OFFSET;
	pmap_pa_start = (KERNTEXTOFF - KERNBASE);
	pmap_pa_end = avail_end;
	mem_clusters[0].start = avail_start;
	mem_clusters[0].size = avail_end - avail_start;
	mem_cluster_cnt++;
	physmem += xen_start_info.nr_pages;
	uvmexp.wired += atop(avail_start);
	/*
	 * initgdt() has to be done before consinit(), so that %fs is properly
	 * initialised. initgdt() uses pmap_kenter_pa so it can't be called
	 * before the above variables are set.
	 */
	initgdt(NULL);
#endif /* XEN */
	consinit();	/* XXX SHOULD NOT BE DONE HERE */

#ifdef DEBUG_MEMLOAD
	printf("mem_cluster_count: %d\n", mem_cluster_cnt);
#endif

	/*
	 * Call pmap initialization to make new kernel address space.
	 * We must do this before loading pages into the VM system.
	 */
	pmap_bootstrap((vaddr_t)atdevbase + IOM_SIZE);

#ifndef XEN
	/*
	 * Check to see if we have a memory map from the BIOS (passed
	 * to us by the boot program.
	 */
	if ((biosmem_implicit || (biosbasemem == 0 && biosextmem == 0)) &&
	    (bim = lookup_bootinfo(BTINFO_MEMMAP)) != NULL && bim->num > 0) {
#ifdef DEBUG_MEMLOAD
		printf("BIOS MEMORY MAP (%d ENTRIES):\n", bim->num);
#endif
		for (x = 0; x < bim->num; x++) {
#ifdef DEBUG_MEMLOAD
			printf("    addr 0x%qx  size 0x%qx  type 0x%x\n",
			    bim->entry[x].addr,
			    bim->entry[x].size,
			    bim->entry[x].type);
#endif

			/*
			 * If the segment is not memory, skip it.
			 */
			switch (bim->entry[x].type) {
			case BIM_Memory:
			case BIM_ACPI:
			case BIM_NVS:
				break;
			default:
				continue;
			}

			/*
			 * If the segment is smaller than a page, skip it.
			 */
			if (bim->entry[x].size < NBPG) {
				continue;
			}

			/*
			 * Sanity check the entry.
			 * XXX Need to handle uint64_t in extent code
			 * XXX and 64-bit physical addresses in i386
			 * XXX port.
			 */
			seg_start = bim->entry[x].addr;
			seg_end = bim->entry[x].addr + bim->entry[x].size;

			/*
			 *   Avoid Compatibility Holes.
			 * XXX  Holes within memory space that allow access
			 * XXX to be directed to the PC-compatible frame buffer
			 * XXX (0xa0000-0xbffff),to adapter ROM space
			 * XXX (0xc0000-0xdffff), and to system BIOS space
			 * XXX (0xe0000-0xfffff).
			 * XXX  Some laptop(for example,Toshiba Satellite2550X)
			 * XXX report this area and occurred problems,
			 * XXX so we avoid this area.
			 */
			if (seg_start < 0x100000 && seg_end > 0xa0000) {
				printf("WARNING: memory map entry overlaps "
				    "with ``Compatibility Holes'': "
				    "0x%qx/0x%qx/0x%x\n", seg_start,
				    seg_end - seg_start, bim->entry[x].type);
				add_mem_cluster(seg_start, 0xa0000,
				    bim->entry[x].type);
				add_mem_cluster(0x100000, seg_end,
				    bim->entry[x].type);
			} else
				add_mem_cluster(seg_start, seg_end,
				    bim->entry[x].type);
		}
	}

	/*
	 * If the loop above didn't find any valid segment, fall back to
	 * former code.
	 */
	if (mem_cluster_cnt == 0) {
		/*
		 * Allocate the physical addresses used by RAM from the iomem
		 * extent map.  This is done before the addresses are
		 * page rounded just to make sure we get them all.
		 */
		if (extent_alloc_region(iomem_ex, 0, KBTOB(biosbasemem),
		    EX_NOWAIT)) {
			/* XXX What should we do? */
			printf("WARNING: CAN'T ALLOCATE BASE MEMORY FROM "
			    "IOMEM EXTENT MAP!\n");
		}
		mem_clusters[0].start = 0;
		mem_clusters[0].size = trunc_page(KBTOB(biosbasemem));
		physmem += atop(mem_clusters[0].size);
		if (extent_alloc_region(iomem_ex, IOM_END, KBTOB(biosextmem),
		    EX_NOWAIT)) {
			/* XXX What should we do? */
			printf("WARNING: CAN'T ALLOCATE EXTENDED MEMORY FROM "
			    "IOMEM EXTENT MAP!\n");
		}
#if NISADMA > 0
		/*
		 * Some motherboards/BIOSes remap the 384K of RAM that would
		 * normally be covered by the ISA hole to the end of memory
		 * so that it can be used.  However, on a 16M system, this
		 * would cause bounce buffers to be allocated and used.
		 * This is not desirable behaviour, as more than 384K of
		 * bounce buffers might be allocated.  As a work-around,
		 * we round memory down to the nearest 1M boundary if
		 * we're using any isadma devices and the remapped memory
		 * is what puts us over 16M.
		 */
		if (biosextmem > (15*1024) && biosextmem < (16*1024)) {
			char pbuf[9];

			format_bytes(pbuf, sizeof(pbuf),
			    biosextmem - (15*1024));
			printf("Warning: ignoring %s of remapped memory\n",
			    pbuf);
			biosextmem = (15*1024);
		}
#endif
		mem_clusters[1].start = IOM_END;
		mem_clusters[1].size = trunc_page(KBTOB(biosextmem));
		physmem += atop(mem_clusters[1].size);

		mem_cluster_cnt = 2;

		avail_end = IOM_END + trunc_page(KBTOB(biosextmem));
	}
	/*
	 * If we have 16M of RAM or less, just put it all on
	 * the default free list.  Otherwise, put the first
	 * 16M of RAM on a lower priority free list (so that
	 * all of the ISA DMA'able memory won't be eaten up
	 * first-off).
	 */
	if (avail_end <= (16 * 1024 * 1024))
		first16q = VM_FREELIST_DEFAULT;
	else
		first16q = VM_FREELIST_FIRST16;

	/* Make sure the end of the space used by the kernel is rounded. */
	first_avail = round_page(first_avail);

	/*
	 * Now, load the memory clusters (which have already been
	 * rounded and truncated) into the VM system.
	 *
	 * NOTE: WE ASSUME THAT MEMORY STARTS AT 0 AND THAT THE KERNEL
	 * IS LOADED AT IOM_END (1M).
	 */
	for (x = 0; x < mem_cluster_cnt; x++) {
		seg_start = mem_clusters[x].start;
		seg_end = mem_clusters[x].start + mem_clusters[x].size;
		seg_start1 = 0;
		seg_end1 = 0;

		/*
		 * Skip memory before our available starting point.
		 */
		if (seg_end <= avail_start)
			continue;

		if (avail_start >= seg_start && avail_start < seg_end) {
			if (seg_start != 0)
				panic("init386: memory doesn't start at 0");
			seg_start = avail_start;
			if (seg_start == seg_end)
				continue;
		}

		/*
		 * If this segment contains the kernel, split it
		 * in two, around the kernel.
		 */
		if (seg_start <= IOM_END && first_avail <= seg_end) {
			seg_start1 = first_avail;
			seg_end1 = seg_end;
			seg_end = IOM_END;
		}

		/* First hunk */
		if (seg_start != seg_end) {
			if (seg_start < (16 * 1024 * 1024) &&
			    first16q != VM_FREELIST_DEFAULT) {
				uint64_t tmp;

				if (seg_end > (16 * 1024 * 1024))
					tmp = (16 * 1024 * 1024);
				else
					tmp = seg_end;

				if (tmp != seg_start) {
#ifdef DEBUG_MEMLOAD
					printf("loading 0x%qx-0x%qx "
					    "(0x%lx-0x%lx)\n",
				    	    seg_start, tmp,
				  	    atop(seg_start), atop(tmp));
#endif
					uvm_page_physload(atop(seg_start),
				    	    atop(tmp), atop(seg_start),
				    	    atop(tmp), first16q);
				}
				seg_start = tmp;
			}

			if (seg_start != seg_end) {
#ifdef DEBUG_MEMLOAD
				printf("loading 0x%qx-0x%qx (0x%lx-0x%lx)\n",
				    seg_start, seg_end,
				    atop(seg_start), atop(seg_end));
#endif
				uvm_page_physload(atop(seg_start),
				    atop(seg_end), atop(seg_start),
				    atop(seg_end), VM_FREELIST_DEFAULT);
			}
		}

		/* Second hunk */
		if (seg_start1 != seg_end1) {
			if (seg_start1 < (16 * 1024 * 1024) &&
			    first16q != VM_FREELIST_DEFAULT) {
				uint64_t tmp;

				if (seg_end1 > (16 * 1024 * 1024))
					tmp = (16 * 1024 * 1024);
				else
					tmp = seg_end1;

				if (tmp != seg_start1) {
#ifdef DEBUG_MEMLOAD
					printf("loading 0x%qx-0x%qx "
					    "(0x%lx-0x%lx)\n",
				    	    seg_start1, tmp,
				    	    atop(seg_start1), atop(tmp));
#endif
					uvm_page_physload(atop(seg_start1),
				    	    atop(tmp), atop(seg_start1),
				    	    atop(tmp), first16q);
				}
				seg_start1 = tmp;
			}

			if (seg_start1 != seg_end1) {
#ifdef DEBUG_MEMLOAD
				printf("loading 0x%qx-0x%qx (0x%lx-0x%lx)\n",
				    seg_start1, seg_end1,
				    atop(seg_start1), atop(seg_end1));
#endif
				uvm_page_physload(atop(seg_start1),
				    atop(seg_end1), atop(seg_start1),
				    atop(seg_end1), VM_FREELIST_DEFAULT);
			}
		}
	}
#else /* !XEN */
	XENPRINTK(("load the memory cluster %p(%d) - %p(%ld)\n",
	    (void *)(long)avail_start, (int)atop(avail_start),
	    (void *)(long)avail_end, (int)atop(avail_end)));
	uvm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end),
	    VM_FREELIST_DEFAULT);
#endif /* !XEN */

	init386_msgbuf();

#ifndef XEN
	/*
	 * XXX Remove this
	 *
	 * Setup a temporary Page Table Entry to allow identity mappings of
	 * the real mode address. This is required by:
	 * - bioscall
	 * - MP bootstrap
	 * - ACPI wakecode
	 */
	init386_pte0();

#if NBIOSCALL > 0
	KASSERT(biostramp_image_size <= PAGE_SIZE);
	pmap_kenter_pa((vaddr_t)BIOSTRAMP_BASE,	/* virtual */
		       (paddr_t)BIOSTRAMP_BASE,	/* physical */
		       VM_PROT_ALL);		/* protection */
	pmap_update(pmap_kernel());
	memcpy((void *)BIOSTRAMP_BASE, biostramp_image, biostramp_image_size);
#endif
#endif /* !XEN */

	pmap_kenter_pa(idt_vaddr, idt_paddr, VM_PROT_READ|VM_PROT_WRITE);
	pmap_update(pmap_kernel());
	memset((void *)idt_vaddr, 0, PAGE_SIZE);


#ifndef XEN
	idt_init();

	idt = (struct gate_descriptor *)idt_vaddr;
	pmap_kenter_pa(pentium_idt_vaddr, idt_paddr, VM_PROT_READ);
	pmap_update(pmap_kernel());
	pentium_idt = (union descriptor *)pentium_idt_vaddr;

	tgdt = gdt;
	gdt = (union descriptor *)
		    ((char *)idt + NIDT * sizeof (struct gate_descriptor));
	ldt = gdt + NGDT;

	memcpy(gdt, tgdt, NGDT*sizeof(*gdt));

	setsegment(&gdt[GLDT_SEL].sd, ldt, NLDT * sizeof(ldt[0]) - 1,
	    SDT_SYSLDT, SEL_KPL, 0, 0);
#else
	HYPERVISOR_set_callbacks(
	    GSEL(GCODE_SEL, SEL_KPL), (unsigned long)hypervisor_callback,
	    GSEL(GCODE_SEL, SEL_KPL), (unsigned long)failsafe_callback);

	ldt = (union descriptor *)idt_vaddr;
#endif /* XEN */

	/* make ldt gates and memory segments */
	setgate(&ldt[LSYS5CALLS_SEL].gd, &IDTVEC(osyscall), 1,
	    SDT_SYS386CGT, SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));

	ldt[LUCODE_SEL] = gdt[GUCODE_SEL];
	ldt[LUCODEBIG_SEL] = gdt[GUCODEBIG_SEL];
	ldt[LUDATA_SEL] = gdt[GUDATA_SEL];
	ldt[LSOL26CALLS_SEL] = ldt[LBSDICALLS_SEL] = ldt[LSYS5CALLS_SEL];

#ifndef XEN
	/* exceptions */
	for (x = 0; x < 32; x++) {
		idt_vec_reserve(x);
		setgate(&idt[x], IDTVEC(exceptions)[x], 0, SDT_SYS386TGT,
		    (x == 3 || x == 4) ? SEL_UPL : SEL_KPL,
		    GSEL(GCODE_SEL, SEL_KPL));
	}

	/* new-style interrupt gate for syscalls */
	idt_vec_reserve(128);
	setgate(&idt[128], &IDTVEC(syscall), 0, SDT_SYS386TGT, SEL_UPL,
	    GSEL(GCODE_SEL, SEL_KPL));
#ifdef COMPAT_SVR4
	idt_vec_reserve(0xd2);
	setgate(&idt[0xd2], &IDTVEC(svr4_fasttrap), 0, SDT_SYS386TGT,
	    SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));
#endif /* COMPAT_SVR4 */

	setregion(&region, gdt, NGDT * sizeof(gdt[0]) - 1);
	lgdt(&region);

	cpu_init_idt();
#else /* !XEN */
	memset(xen_idt, 0, sizeof(trap_info_t) * MAX_XEN_IDT);
	xen_idt_idx = 0;
	for (x = 0; x < 32; x++) {
		KASSERT(xen_idt_idx < MAX_XEN_IDT);
		xen_idt[xen_idt_idx].vector = x;
		xen_idt[xen_idt_idx].flags =
			(x == 3 || x == 4) ? SEL_UPL : SEL_XEN;
		xen_idt[xen_idt_idx].cs = GSEL(GCODE_SEL, SEL_KPL);
		xen_idt[xen_idt_idx].address =
			(uint32_t)IDTVEC(exceptions)[x];
		xen_idt_idx++;
	}
	KASSERT(xen_idt_idx < MAX_XEN_IDT);
	xen_idt[xen_idt_idx].vector = 128;
	xen_idt[xen_idt_idx].flags = SEL_UPL;
	xen_idt[xen_idt_idx].cs = GSEL(GCODE_SEL, SEL_KPL);
	xen_idt[xen_idt_idx].address = (uint32_t)&IDTVEC(syscall);
	xen_idt_idx++;
#ifdef COMPAT_SVR4
	KASSERT(xen_idt_idx < MAX_XEN_IDT);
	xen_idt[xen_idt_idx].vector = 0xd2;
	xen_idt[xen_idt_idx].flags = SEL_UPL;
	xen_idt[xen_idt_idx].cs = GSEL(GCODE_SEL, SEL_KPL);
	xen_idt[xen_idt_idx].address = (uint32_t)&IDTVEC(svr4_fasttrap);
	xen_idt_idx++;
#endif /* COMPAT_SVR4 */
	lldt(GSEL(GLDT_SEL, SEL_KPL));

	XENPRINTF(("HYPERVISOR_set_trap_table %p\n", xen_idt));
	if (HYPERVISOR_set_trap_table(xen_idt))
		panic("HYPERVISOR_set_trap_table %p failed\n", xen_idt);
#endif /* XEN */

	init386_ksyms();

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef IPKDB
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif
#ifdef KGDB
	kgdb_port_init();
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif

#if NMCA > 0
	/* check for MCA bus, needed to be done before ISA stuff - if
	 * MCA is detected, ISA needs to use level triggered interrupts
	 * by default */
	mca_busprobe();
#endif

#ifdef XEN
	XENPRINTF(("events_default_setup\n"));
	events_default_setup();
#else
	intr_default_setup();
#endif

	splraise(IPL_IPI);
	x86_enable_intr();

	if (physmem < btoc(2 * 1024 * 1024)) {
		printf("warning: too little memory available; "
		       "have %lu bytes, want %lu bytes\n"
		       "running in degraded mode\n"
		       "press a key to confirm\n\n",
		       ptoa(physmem), 2*1024*1024UL);
		cngetc();
	}
}

#ifdef COMPAT_NOMID
static int
exec_nomid(struct lwp *l, struct exec_package *epp)
{
	int error;
	u_long midmag, magic;
	u_short mid;
	struct exec *execp = epp->ep_hdr;

	/* check on validity of epp->ep_hdr performed by exec_out_makecmds */

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0xffff;
	magic = midmag & 0xffff;

	if (magic == 0) {
		magic = (execp->a_midmag & 0xffff);
		mid = MID_ZERO;
	}

	midmag = mid << 16 | magic;

	switch (midmag) {
	case (MID_ZERO << 16) | ZMAGIC:
		/*
		 * 386BSD's ZMAGIC format:
		 */
		error = exec_aout_prep_oldzmagic(l, epp);
		break;

	case (MID_ZERO << 16) | QMAGIC:
		/*
		 * BSDI's QMAGIC format:
		 * same as new ZMAGIC format, but with different magic number
		 */
		error = exec_aout_prep_zmagic(l, epp);
		break;

	case (MID_ZERO << 16) | NMAGIC:
		/*
		 * BSDI's NMAGIC format:
		 * same as NMAGIC format, but with different magic number
		 * and with text starting at 0.
		 */
		error = exec_aout_prep_oldnmagic(l, epp);
		break;

	case (MID_ZERO << 16) | OMAGIC:
		/*
		 * BSDI's OMAGIC format:
		 * same as OMAGIC format, but with different magic number
		 * and with text starting at 0.
		 */
		error = exec_aout_prep_oldomagic(l, epp);
		break;

	default:
		error = ENOEXEC;
	}

	return error;
}
#endif

/*
 * cpu_exec_aout_makecmds():
 *	CPU-dependent a.out format hook for execve().
 *
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * On the i386, old (386bsd) ZMAGIC binaries and BSDI QMAGIC binaries
 * if COMPAT_NOMID is given as a kernel option.
 */
int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
	int error = ENOEXEC;

#ifdef COMPAT_NOMID
	if ((error = exec_nomid(l, epp)) == 0)
		return error;
#else
	(void) l;
	(void) epp;
#endif /* ! COMPAT_NOMID */

	return error;
}

#include <dev/ic/mc146818reg.h>		/* for NVRAM POST */
#include <i386/isa/nvram.h>		/* for NVRAM POST */

void
cpu_reset()
{
#ifdef XEN
	HYPERVISOR_reboot();
	for (;;);
#else /* XEN */
	struct region_descriptor region;

	x86_disable_intr();
#ifdef XBOX
	if (arch_i386_is_xbox) {
		xbox_reboot();
		for (;;);
	}
#endif

	/*
	 * Ensure the NVRAM reset byte contains something vaguely sane.
	 */

	outb(IO_RTC, NVRAM_RESET);
	outb(IO_RTC+1, NVRAM_RESET_RST);

	/*
	 * Reset AMD Geode SC1100.
	 *
	 * 1) Write PCI Configuration Address Register (0xcf8) to
	 *    select Function 0, Register 0x44: Bridge Configuration,
	 *    GPIO and LPC Configuration Register Space, Reset
	 *    Control Register.
	 *
	 * 2) Write 0xf to PCI Configuration Data Register (0xcfc)
	 *    to reset IDE controller, IDE bus, and PCI bus, and
	 *    to trigger a system-wide reset.
	 * 
	 * See AMD Geode SC1100 Processor Data Book, Revision 2.0,
	 * sections 6.3.1, 6.3.2, and 6.4.1.
	 */
	if (cpu_info_primary.ci_signature == 0x540) {
		outl(0xcf8, 0x80009044);
		outl(0xcfc, 0xf);
	}

	/*
	 * The keyboard controller has 4 random output pins, one of which is
	 * connected to the RESET pin on the CPU in many PCs.  We tell the
	 * keyboard controller to pulse this line a couple of times.
	 */
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);

	/*
	 * Try to cause a triple fault and watchdog reset by making the IDT
	 * invalid and causing a fault.
	 */
	memset((void *)idt, 0, NIDT * sizeof(idt[0]));
	setregion(&region, idt, NIDT * sizeof(idt[0]) - 1);
	lidt(&region);
	breakpoint();

#if 0
	/*
	 * Try to cause a triple fault and watchdog reset by unmapping the
	 * entire address space and doing a TLB flush.
	 */
	memset((void *)PTD, 0, PAGE_SIZE);
	tlbflush();
#endif

	for (;;);
#endif /* XEN */
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	const struct trapframe *tf = l->l_md.md_regs;
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_eip;

	/* Save register context. */
#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		gr[_REG_GS]  = tf->tf_vm86_gs;
		gr[_REG_FS]  = tf->tf_vm86_fs;
		gr[_REG_ES]  = tf->tf_vm86_es;
		gr[_REG_DS]  = tf->tf_vm86_ds;
		gr[_REG_EFL] = get_vflags(l);
	} else
#endif
	{
		gr[_REG_GS]  = tf->tf_gs;
		gr[_REG_FS]  = tf->tf_fs;
		gr[_REG_ES]  = tf->tf_es;
		gr[_REG_DS]  = tf->tf_ds;
		gr[_REG_EFL] = tf->tf_eflags;
	}
	gr[_REG_EDI]    = tf->tf_edi;
	gr[_REG_ESI]    = tf->tf_esi;
	gr[_REG_EBP]    = tf->tf_ebp;
	gr[_REG_EBX]    = tf->tf_ebx;
	gr[_REG_EDX]    = tf->tf_edx;
	gr[_REG_ECX]    = tf->tf_ecx;
	gr[_REG_EAX]    = tf->tf_eax;
	gr[_REG_EIP]    = tf->tf_eip;
	gr[_REG_CS]     = tf->tf_cs;
	gr[_REG_ESP]    = tf->tf_esp;
	gr[_REG_UESP]   = tf->tf_esp;
	gr[_REG_SS]     = tf->tf_ss;
	gr[_REG_TRAPNO] = tf->tf_trapno;
	gr[_REG_ERR]    = tf->tf_err;

	if ((ras_eip = (__greg_t)ras_lookup(l->l_proc,
	    (void *) gr[_REG_EIP])) != -1)
		gr[_REG_EIP] = ras_eip;

	*flags |= _UC_CPU;

	/* Save floating point register context, if any. */
	if ((l->l_md.md_flags & MDL_USEDFPU) != 0) {
#if NNPX > 0
		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 * XXX npxsave() also clears the FPU state; depending on the
		 * XXX application this might be a penalty.
		 */
		if (l->l_addr->u_pcb.pcb_fpcpu) {
			npxsave_lwp(l, true);
		}
#endif
		if (i386_use_fxsave) {
			memcpy(&mcp->__fpregs.__fp_reg_set.__fp_xmm_state.__fp_xmm,
			    &l->l_addr->u_pcb.pcb_savefpu.sv_xmm,
			    sizeof (mcp->__fpregs.__fp_reg_set.__fp_xmm_state.__fp_xmm));
			*flags |= _UC_FXSAVE;
		} else {
			memcpy(&mcp->__fpregs.__fp_reg_set.__fpchip_state.__fp_state,
			    &l->l_addr->u_pcb.pcb_savefpu.sv_87,
			    sizeof (mcp->__fpregs.__fp_reg_set.__fpchip_state.__fp_state));
		}
#if 0
		/* Apparently nothing ever touches this. */
		ucp->mcp.mc_fp.fp_emcsts = l->l_addr->u_pcb.pcb_saveemc;
#endif
		*flags |= _UC_FPU;
	}
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_md.md_regs;
	const __greg_t *gr = mcp->__gregs;
	struct proc *p = l->l_proc;

	/* Restore register context, if any. */
	if ((flags & _UC_CPU) != 0) {
#ifdef VM86
		if (gr[_REG_EFL] & PSL_VM) {
			tf->tf_vm86_gs = gr[_REG_GS];
			tf->tf_vm86_fs = gr[_REG_FS];
			tf->tf_vm86_es = gr[_REG_ES];
			tf->tf_vm86_ds = gr[_REG_DS];
			set_vflags(l, gr[_REG_EFL]);
			if (flags & _UC_VM) {
				void syscall_vm86(struct trapframe *);
				l->l_proc->p_md.md_syscall = syscall_vm86;
			}
		} else
#endif
		{
			/*
			 * Check for security violations.  If we're returning
			 * to protected mode, the CPU will validate the segment
			 * registers automatically and generate a trap on
			 * violations.  We handle the trap, rather than doing
			 * all of the checking here.
			 */
			if (((gr[_REG_EFL] ^ tf->tf_eflags) & PSL_USERSTATIC) ||
			    !USERMODE(gr[_REG_CS], gr[_REG_EFL])) {
				printf("cpu_setmcontext error: uc EFL: 0x%08x"
				    " tf EFL: 0x%08x uc CS: 0x%x\n",
				    gr[_REG_EFL], tf->tf_eflags, gr[_REG_CS]);
				return (EINVAL);
			}
			tf->tf_gs = gr[_REG_GS];
			tf->tf_fs = gr[_REG_FS];
			tf->tf_es = gr[_REG_ES];
			tf->tf_ds = gr[_REG_DS];
			/* Only change the user-alterable part of eflags */
			tf->tf_eflags &= ~PSL_USER;
			tf->tf_eflags |= (gr[_REG_EFL] & PSL_USER);
		}
		tf->tf_edi    = gr[_REG_EDI];
		tf->tf_esi    = gr[_REG_ESI];
		tf->tf_ebp    = gr[_REG_EBP];
		tf->tf_ebx    = gr[_REG_EBX];
		tf->tf_edx    = gr[_REG_EDX];
		tf->tf_ecx    = gr[_REG_ECX];
		tf->tf_eax    = gr[_REG_EAX];
		tf->tf_eip    = gr[_REG_EIP];
		tf->tf_cs     = gr[_REG_CS];
		tf->tf_esp    = gr[_REG_UESP];
		tf->tf_ss     = gr[_REG_SS];
	}

	/* Restore floating point register context, if any. */
	if ((flags & _UC_FPU) != 0) {
#if NNPX > 0
		/*
		 * If we were using the FPU, forget that we were.
		 */
		if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
			npxsave_lwp(l, false);
#endif
		if (flags & _UC_FXSAVE) {
			if (i386_use_fxsave) {
				memcpy(
					&l->l_addr->u_pcb.pcb_savefpu.sv_xmm,
					&mcp->__fpregs.__fp_reg_set.__fp_xmm_state.__fp_xmm,
					sizeof (&l->l_addr->u_pcb.pcb_savefpu.sv_xmm));
			} else {
				/* This is a weird corner case */
				process_xmm_to_s87((struct savexmm *)
				    &mcp->__fpregs.__fp_reg_set.__fp_xmm_state.__fp_xmm,
				    &l->l_addr->u_pcb.pcb_savefpu.sv_87);
			}
		} else {
			if (i386_use_fxsave) {
				process_s87_to_xmm((struct save87 *)
				    &mcp->__fpregs.__fp_reg_set.__fpchip_state.__fp_state,
				    &l->l_addr->u_pcb.pcb_savefpu.sv_xmm);
			} else {
				memcpy(&l->l_addr->u_pcb.pcb_savefpu.sv_87,
				    &mcp->__fpregs.__fp_reg_set.__fpchip_state.__fp_state,
				    sizeof (l->l_addr->u_pcb.pcb_savefpu.sv_87));
			}
		}
		/* If not set already. */
		l->l_md.md_flags |= MDL_USEDFPU;
#if 0
		/* Apparently unused. */
		l->l_addr->u_pcb.pcb_saveemc = mcp->mc_fp.fp_emcsts;
#endif
	}
	mutex_enter(&p->p_smutex);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(&p->p_smutex);
	return (0);
}

void
cpu_initclocks()
{

	(*initclock_func)();
}
