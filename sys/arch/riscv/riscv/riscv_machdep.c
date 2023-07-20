/*	$NetBSD: riscv_machdep.c,v 1.31 2023/07/10 07:04:20 rin Exp $	*/

/*-
 * Copyright (c) 2014, 2019, 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry, and by Nick Hudson.
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

#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_riscv_debug.h"

#include <sys/cdefs.h>
__RCSID("$NetBSD: riscv_machdep.c,v 1.31 2023/07/10 07:04:20 rin Exp $");

#include <sys/param.h>

#include <sys/asan.h>
#include <sys/boot_flag.h>
#include <sys/cpu.h>
#include <sys/exec.h>
#include <sys/kmem.h>
#include <sys/ktrace.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/optstr.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/cons.h>
#include <uvm/uvm_extern.h>

#include <riscv/frame.h>
#include <riscv/locore.h>
#include <riscv/machdep.h>
#include <riscv/pte.h>
#include <riscv/sbi.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_boot.h>
#include <dev/fdt/fdt_memory.h>
#include <dev/fdt/fdt_private.h>

int cpu_printfataltraps = 1;
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;

#ifdef VERBOSE_INIT_RISCV
#define	VPRINTF(...)	printf(__VA_ARGS__)
#else
#define	VPRINTF(...)	__nothing
#endif

/* 64 should be enough, even for a ZFS UUID */
#define	MAX_BOOT_DEV_STR	64

char bootdevstr[MAX_BOOT_DEV_STR] = "";
char *boot_args = NULL;

paddr_t physical_start;
paddr_t physical_end;

static void
earlyconsputc(dev_t dev, int c)
{
	uartputc(c);
}

static int
earlyconsgetc(dev_t dev)
{
	return uartgetc();
}

static struct consdev earlycons = {
	.cn_putc = earlyconsputc,
	.cn_getc = earlyconsgetc,
	.cn_pollc = nullcnpollc,
};

struct vm_map *phys_map;

struct trapframe cpu_ddb_regs;
const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
#ifdef FPE
	[PCU_FPU] = &pcu_fpu_ops,
#endif
};

/*
 * Used by PHYSTOV and VTOPHYS -- Will be set be BSS is zeroed so
 * keep it in data
 */
unsigned long kern_vtopdiff __attribute__((__section__(".data")));


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
}

void
delay(unsigned long us)
{
	const uint32_t cycles_per_us = curcpu()->ci_data.cpu_cc_freq / 1000000;
	const uint64_t cycles = (uint64_t)us * cycles_per_us;
	const uint64_t finish = csr_cycle_read() + cycles;

	while (csr_cycle_read() < finish) {
		/* spin, baby spin */
	}
}

#ifdef MODULAR
/*
 * Push any modules loaded by the boot loader.
 */
void
module_init_md(void)
{
}
#endif /* MODULAR */

/*
 * Set registers on exec.
 * Clear all registers except sp, pc.
 * sp is set to the stack pointer passed in.  pc is set to the entry
 * point given by the exec_package passed in.
 */
void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;

	memset(tf, 0, sizeof(*tf));
	tf->tf_sp = (intptr_t)stack_align(stack);
	tf->tf_pc = (intptr_t)pack->ep_entry & ~1;
#ifdef _LP64
	tf->tf_sr = (p->p_flag & PK_32) ? SR_USER32 : SR_USER64;
#else
	tf->tf_sr = SR_USER;
#endif

	// Set up arguments for ___start(cleanup, ps_strings)
	tf->tf_a0 = 0;			// cleanup
	tf->tf_a1 = p->p_psstrp;	// ps_strings

	/*
	 * Must have interrupts disabled for exception return.
	 * Must be switching to user mode.
	 * Must enable interrupts after sret.
	 */
	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SIE) == 0);
	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SPP) == 0);
	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SPIE) != 0);
}

void
md_child_return(struct lwp *l)
{
	struct trapframe * const tf = lwp_trapframe(l);

	tf->tf_a0 = 0;
	tf->tf_a1 = 1;
#ifdef FPE
	/* Disable FP as we can't be using it (yet). */
	tf->tf_sr &= ~SR_FS;
#endif

	/*
	 * Must have interrupts disabled for exception return.
	 * Must be switching to user mode.
	 * Must enable interrupts after sret.
	 */

	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SIE) == 0);
	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SPP) == 0);
	KASSERT(__SHIFTOUT(tf->tf_sr, SR_SPIE) != 0);

	userret(l);
}

void
cpu_spawn_return(struct lwp *l)
{
	userret(l);
}

/*
 * Start a new LWP
 */
void
startlwp(void *arg)
{
	ucontext_t * const uc = arg;
	lwp_t * const l = curlwp;
	int error __diagused;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(*uc));
	userret(l);
}

// We've worked hard to make sure struct reg and __gregset_t are the same.
// Ditto for struct fpreg and fregset_t.

#ifdef _LP64
CTASSERT(sizeof(struct reg) == sizeof(__gregset_t));
#endif
CTASSERT(sizeof(struct fpreg) == sizeof(__fregset_t));

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	const struct trapframe * const tf = l->l_md.md_utf;

	/* Save register context. */
	*(struct reg *)mcp->__gregs = tf->tf_regs;

	*flags |= _UC_CPU | _UC_TLSBASE;

	/* Save floating point register context, if any. */
	KASSERT(l == curlwp);
	if (fpu_valid_p(l)) {
		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 */
		fpu_save(l);

		struct pcb * const pcb = lwp_getpcb(l);
		*(struct fpreg *)mcp->__fregs = pcb->pcb_fpregs;
		*flags |= _UC_FPU;
	}
}

int
cpu_mcontext_validate(struct lwp *l, const mcontext_t *mcp)
{
	/*
	 * Verify that at least the PC and SP are user addresses.
	 */
	if ((intptr_t) mcp->__gregs[_REG_PC] < 0
	    || (intptr_t) mcp->__gregs[_REG_SP] < 0
	    || (mcp->__gregs[_REG_PC] & 1))
		return EINVAL;

	return 0;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;
	const __greg_t * const gr = mcp->__gregs;
	int error;

	/* Restore register context, if any. */
	if (flags & _UC_CPU) {
		error = cpu_mcontext_validate(l, mcp);
		if (error)
			return error;

		/* Save register context. */
		tf->tf_regs = *(const struct reg *)gr;
	}

	/* Restore the private thread context */
	if (flags & _UC_TLSBASE) {
		lwp_setprivate(l, (void *)(intptr_t)mcp->__gregs[_X_TP]);
	}

	/* Restore floating point register context, if any. */
	if (flags & _UC_FPU) {
		KASSERT(l == curlwp);
		/* Tell PCU we are replacing the FPU contents. */
		fpu_replace(l);

		/*
		 * The PCB FP regs struct includes the FP CSR, so use the
		 * proper size of fpreg when copying.
		 */
		struct pcb * const pcb = lwp_getpcb(l);
		pcb->pcb_fpregs = *(const struct fpreg *)mcp->__fregs;
	}

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return 0;
}

void
cpu_need_resched(struct cpu_info *ci, struct lwp *l, int flags)
{
	KASSERT(kpreempt_disabled());

	if ((flags & RESCHED_KPREEMPT) != 0) {
#ifdef __HAVE_PREEMPTION
		if ((flags & RESCHED_REMOTE) != 0) {
			cpu_send_ipi(ci, IPI_KPREEMPT);
		} else {
			softint_trigger(SOFTINT_KPREEMPT);
		}
#endif
		return;
	}
	if ((flags & RESCHED_REMOTE) != 0) {
#ifdef MULTIPROCESSOR
		cpu_send_ipi(ci, IPI_AST);
#endif
	} else {
		l->l_md.md_astpending = 1;	/* force call to ast() */
	}
}

void
cpu_signotify(struct lwp *l)
{
	KASSERT(kpreempt_disabled());
#ifdef __HAVE_FAST_SOFTINTS
	KASSERT(lwp_locked(l, NULL));
#endif

	if (l->l_cpu != curcpu()) {
#ifdef MULTIPROCESSOR
		cpu_send_ipi(l->l_cpu, IPI_AST);
#endif
	} else {
		l->l_md.md_astpending = 1; 	/* force call to ast() */
	}
}

void
cpu_need_proftick(struct lwp *l)
{
	KASSERT(kpreempt_disabled());
	KASSERT(l->l_cpu == curcpu());

	l->l_pflag |= LP_OWEUPC;
	l->l_md.md_astpending = 1;		/* force call to ast() */
}


/* Sync the discs, unmount the filesystems, and adjust the todr */
static void
bootsync(void)
{
	static bool bootsyncdone = false;

	if (bootsyncdone)
		return;

	bootsyncdone = true;

	/* Make sure we can still manage to do things */
	if ((csr_sstatus_read() & SR_SIE) == 0) {
		/*
		 * If we get here then boot has been called without RB_NOSYNC
		 * and interrupts were disabled. This means the boot() call
		 * did not come from a user process e.g. shutdown, but must
		 * have come from somewhere in the kernel.
		 */
		ENABLE_INTERRUPTS();
		printf("Warning interrupts disabled during boot()\n");
	}

	vfs_shutdown();

	resettodr();
}


void
cpu_reboot(int howto, char *bootstr)
{

	/*
	 * If RB_NOSYNC was not specified sync the discs.
	 * Note: Unless cold is set to 1 here, syslogd will die during the
	 * unmount.  It looks like syslogd is getting woken up only to find
	 * that it cannot page part of the binary in as the filesystem has
	 * been unmounted.
	 */
	if ((howto & RB_NOSYNC) == 0)
		bootsync();

#if 0
	/* Disable interrupts. */
	const int s = splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

	splx(s);
#endif

	pmf_system_shutdown(boothowto);

	/* Say NO to interrupts for good */
	splhigh();

	/* Run any shutdown hooks */
	doshutdownhooks();

	/* Make sure IRQ's are disabled */
	DISABLE_INTERRUPTS();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* for proper keyboard command handling */
		if (cngetc() == 0) {
			/* no console attached, so just hlt */
			printf("No keyboard - cannot reboot after all.\n");
			goto spin;
		}
		cnpollc(0);
	}

	printf("rebooting...\n");

	sbi_system_reset(SBI_RESET_TYPE_COLDREBOOT, SBI_RESET_REASON_NONE);
spin:
	for (;;) {
		asm volatile("wfi" ::: "memory");
	}
	/* NOTREACHED */
}

void
cpu_dumpconf(void)
{
	// TBD!!
}


int
cpu_lwp_setprivate(lwp_t *l, void *addr)
{
	struct trapframe * const tf = lwp_trapframe(l);

	tf->tf_reg[_REG_TP] = (register_t)addr;

	return 0;
}


void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[10];	/* "999999 MB" -- But Sv39 is max 512GB */

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, FALSE, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvm_availmem(false)));
	printf("avail memory = %s\n", pbuf);

#ifdef MULTIPROCESSOR
	kcpuset_create(&cpus_halted, true);
	KASSERT(cpus_halted != NULL);

	kcpuset_create(&cpus_hatched, true);
	KASSERT(cpus_hatched != NULL);

	kcpuset_create(&cpus_paused, true);
	KASSERT(cpus_paused != NULL);

	kcpuset_create(&cpus_resumed, true);
	KASSERT(cpus_resumed != NULL);

	kcpuset_create(&cpus_running, true);
	KASSERT(cpus_running != NULL);

	kcpuset_set(cpus_hatched, cpu_number());
	kcpuset_set(cpus_running, cpu_number());
#endif

	fdtbus_intr_init();

	fdt_setup_rndseed();
	fdt_setup_efirng();
}

static void
riscv_add_memory(const struct fdt_memory *m, void *arg)
{
	paddr_t first = atop(m->start);
	paddr_t last = atop(m->end);
	int freelist = VM_FREELIST_DEFAULT;

	VPRINTF("adding %#16" PRIxPADDR " - %#16" PRIxPADDR"  to freelist %d\n",
	    m->start, m->end, freelist);

	uvm_page_physload(first, last, first, last, freelist);
	physmem += last - first;
}


static void
cpu_kernel_vm_init(paddr_t memory_start, paddr_t memory_end)
{
	extern char __kernel_text[];
	extern char _end[];

	vaddr_t kernstart = trunc_page((vaddr_t)__kernel_text);
	vaddr_t kernend = round_page((vaddr_t)_end);
	paddr_t kernstart_phys = KERN_VTOPHYS(kernstart);
	paddr_t kernend_phys = KERN_VTOPHYS(kernend);

	VPRINTF("%s: kernel phys start %#" PRIxPADDR " end %#" PRIxPADDR "\n",
	    __func__, kernstart_phys, kernend_phys);
	fdt_memory_remove_range(kernstart_phys,
	    kernend_phys - kernstart_phys);

	/*
	 * Don't give these pages to UVM.
	 *
	 * cpu_kernel_vm_init need to create proper tables then the following
	 * will be true.
	 *
	 * Now we have APs started the pages used for stacks and L1PT can
	 * be given to uvm
	 */
	extern char const __start__init_memory[];
	extern char const __stop__init_memory[] __weak;
	if (__start__init_memory != __stop__init_memory) {
		const paddr_t spa = KERN_VTOPHYS((vaddr_t)__start__init_memory);
		const paddr_t epa = KERN_VTOPHYS((vaddr_t)__stop__init_memory);

		VPRINTF("%s: init   phys start %#" PRIxPADDR
		    " end %#" PRIxPADDR "\n", __func__, spa, epa);
		fdt_memory_remove_range(spa, epa - spa);
	}

#ifdef _LP64
	paddr_t pa = memory_start & ~XSEGOFSET;
	pmap_direct_base = RISCV_DIRECTMAP_START;
	extern pd_entry_t l2_pte[PAGE_SIZE / sizeof(pd_entry_t)];


	const vsize_t vshift = XSEGSHIFT;
	const vaddr_t pdetab_mask = PMAP_PDETABSIZE - 1;
	const vsize_t inc = 1UL << vshift;

	const vaddr_t sva = RISCV_DIRECTMAP_START + pa;
	const vaddr_t eva = RISCV_DIRECTMAP_END;
	const size_t sidx = (sva >> vshift) & pdetab_mask;
	const size_t eidx = (eva >> vshift) & pdetab_mask;

	/* Allocate gigapages covering all physical memory in the direct map. */
	for (size_t i = sidx; i < eidx && pa < memory_end; i++, pa += inc) {
		l2_pte[i] = PA_TO_PTE(pa) | PTE_KERN | PTE_HARDWIRED | PTE_RW;
		VPRINTF("dm:   %p :  %#" PRIxPADDR "\n", &l2_pte[i], l2_pte[i]);
	}
#endif
//	pt_dump(printf);
}

static void
riscv_init_lwp0_uarea(void)
{
	extern char lwp0uspace[];

	uvm_lwp_setuarea(&lwp0, (vaddr_t)lwp0uspace);
	memset(&lwp0.l_md, 0, sizeof(lwp0.l_md));
	memset(lwp_getpcb(&lwp0), 0, sizeof(struct pcb));

	struct trapframe *tf = (struct trapframe *)(lwp0uspace + USPACE) - 1;
	memset(tf, 0, sizeof(*tf));

	lwp0.l_md.md_utf = lwp0.l_md.md_ktf = tf;
}


static void
riscv_print_memory(const struct fdt_memory *m, void *arg)
{

	VPRINTF("FDT /memory @ 0x%" PRIx64 " size 0x%" PRIx64 "\n",
	    m->start, m->end - m->start);
}


static void
parse_mi_bootargs(char *args)
{
	int howto;
	bool found, start, skipping;

	if (args == NULL)
		return;

	start = true;
	skipping = false;
	for (char *cp = args; *cp; cp++) {
		/* check for "words" starting with a "-" only */
		if (start) {
			if (*cp == '-') {
				skipping = false;
			} else {
				skipping = true;
			}
			start = false;
			continue;
		}

		if (*cp == ' ') {
			start = true;
			skipping = false;
			continue;
		}

		if (skipping) {
			continue;
		}

		/* Check valid boot flags */
		howto = 0;
		BOOT_FLAG(*cp, howto);
		if (!howto)
			printf("bootflag '%c' not recognised\n", *cp);
		else
			boothowto |= howto;
	}

	found = optstr_get(args, "root", bootdevstr, sizeof(bootdevstr));
	if (found) {
		bootspec = bootdevstr;
	}
}


void
init_riscv(register_t hartid, paddr_t dtb)
{

	/* set temporally to work printf()/panic() even before consinit() */
	cn_tab = &earlycons;

	/* Load FDT */
	const vaddr_t dtbva = VM_KERNEL_DTB_BASE + (dtb & (NBSEG - 1));
	void *fdt_data = (void *)dtbva;
	int error = fdt_check_header(fdt_data);
	if (error != 0)
	    panic("fdt_check_header failed: %s", fdt_strerror(error));

	fdtbus_init(fdt_data);

	/* Lookup platform specific backend */
	const struct fdt_platform * const plat = fdt_platform_find();
	if (plat == NULL)
		panic("Kernel does not support this device");

	/* Early console may be available, announce ourselves. */
	VPRINTF("FDT<%p>\n", fdt_data);

	boot_args = fdt_get_bootargs();

	VPRINTF("devmap %p\n", plat->fp_devmap());
	pmap_devmap_bootstrap(0, plat->fp_devmap());

	VPRINTF("bootstrap\n");
	plat->fp_bootstrap();

	/*
	 * If stdout-path is specified on the command line, override the
	 * value in /chosen/stdout-path before initializing console.
	 */
	VPRINTF("stdout\n");
	fdt_update_stdout_path(fdt_data, boot_args);

	/*
	 * Done making changes to the FDT.
	 */
	fdt_pack(fdt_data);

	const uint32_t dtbsize = round_page(fdt_totalsize(fdt_data));

	VPRINTF("fdt size %x/%x\n", dtbsize, fdt_totalsize(fdt_data));

	VPRINTF("consinit ");
	consinit();
	VPRINTF("ok\n");

	/* Talk to the user */
	printf("NetBSD/riscv (fdt) booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_mi_bootargs(mi_bootargs);
#endif

	uint64_t memory_start, memory_end;
	fdt_memory_get(&memory_start, &memory_end);
	physical_start = memory_start;
	physical_end = memory_end;

	fdt_memory_foreach(riscv_print_memory, NULL);

	/* Cannot map memory above largest page number */
	const uint64_t maxppn = __SHIFTOUT_MASK(PTE_PPN) - 1;
	const uint64_t memory_limit = ptoa(maxppn);

	if (memory_end > memory_limit) {
		fdt_memory_remove_range(memory_limit, memory_end);
		memory_end = memory_limit;
	}

	uint64_t memory_size __unused = memory_end - memory_start;

	VPRINTF("%s: memory start %" PRIx64 " end %" PRIx64 " (len %"
	    PRIx64 ")\n", __func__, memory_start, memory_end, memory_size);

	/* Parse ramdisk, rndseed, and firmware's RNG from EFI */
	fdt_probe_initrd();
	fdt_probe_rndseed();
	fdt_probe_efirng();

	fdt_memory_remove_reserved(memory_start, memory_end);

	fdt_memory_remove_range(dtb, dtbsize);
	fdt_reserve_initrd();
	fdt_reserve_rndseed();
	fdt_reserve_efirng();

	/* Perform PT build and VM init */
	cpu_kernel_vm_init(memory_start, memory_end);

	VPRINTF("bootargs: %s\n", boot_args);

	parse_mi_bootargs(boot_args);

#ifdef DDB
	if (boothowto & RB_KDB) {
		printf("Entering DDB...\n");
		cpu_Debugger();
	}
#endif

	extern char __kernel_text[];
	extern char _end[];
//	extern char __data_start[];
//	extern char __rodata_start[];

	vaddr_t kernstart = trunc_page((vaddr_t)__kernel_text);
	vaddr_t kernend = round_page((vaddr_t)_end);
	paddr_t kernstart_phys __unused = KERN_VTOPHYS(kernstart);
	paddr_t kernend_phys __unused = KERN_VTOPHYS(kernend);

	vaddr_t kernelvmstart;

	vaddr_t kernstart_mega __unused = MEGAPAGE_TRUNC(kernstart);
	vaddr_t kernend_mega = MEGAPAGE_ROUND(kernend);

	kernelvmstart = kernend_mega;

#if 0
#ifdef MODULAR
#define MODULE_RESERVED_MAX	(1024 * 1024 * 128)
#define MODULE_RESERVED_SIZE	(1024 * 1024 * 32)	/* good enough? */
	module_start = kernelvmstart;
	module_end = kernend_mega + MODULE_RESERVED_SIZE;
	if (module_end >= kernstart_mega + MODULE_RESERVED_MAX)
		module_end = kernstart_mega + MODULE_RESERVED_MAX;
	KASSERT(module_end > kernend_mega);
	kernelvmstart = module_end;
#endif /* MODULAR */
#endif
	KASSERT(kernelvmstart < VM_KERNEL_VM_BASE);

	kernelvmstart = VM_KERNEL_VM_BASE;

	/*
	 * msgbuf is allocated from the top of the last biggest memory block.
	 */
	paddr_t msgbufaddr = 0;

#ifdef _LP64
	/* XXX check all ranges for last one with a big enough hole */
	msgbufaddr = memory_end - MSGBUFSIZE;
	KASSERT(msgbufaddr != 0);	/* no space for msgbuf */
	fdt_memory_remove_range(msgbufaddr, msgbufaddr + MSGBUFSIZE);
	msgbufaddr = RISCV_PA_TO_KVA(msgbufaddr);
	VPRINTF("msgbufaddr = %#lx\n", msgbufaddr);
	initmsgbuf((void *)msgbufaddr, MSGBUFSIZE);
#endif

	KASSERT(msgbufaddr != 0);	/* no space for msgbuf */
#ifdef _LP64
	initmsgbuf((void *)RISCV_PA_TO_KVA(msgbufaddr), MSGBUFSIZE);
#endif

#define	DPRINTF(v)	VPRINTF("%24s = 0x%16lx\n", #v, (unsigned long)v);

	VPRINTF("------------------------------------------\n");
	DPRINTF(kern_vtopdiff);
	DPRINTF(memory_start);
	DPRINTF(memory_end);
	DPRINTF(memory_size);
	DPRINTF(kernstart_phys);
	DPRINTF(kernend_phys)
	DPRINTF(msgbufaddr);
//	DPRINTF(physical_end);
	DPRINTF(VM_MIN_KERNEL_ADDRESS);
	DPRINTF(kernstart_mega);
	DPRINTF(kernstart);
	DPRINTF(kernend);
	DPRINTF(kernend_mega);
#if 0
#ifdef MODULAR
	DPRINTF(module_start);
	DPRINTF(module_end);
#endif
#endif
	DPRINTF(VM_MAX_KERNEL_ADDRESS);
#ifdef _LP64
	DPRINTF(pmap_direct_base);
#endif
	VPRINTF("------------------------------------------\n");

#undef DPRINTF

	uvm_md_init();

	/*
	 * pass memory pages to uvm
	 */
	physmem = 0;
	fdt_memory_foreach(riscv_add_memory, NULL);

	pmap_bootstrap(kernelvmstart, VM_MAX_KERNEL_ADDRESS);

	kasan_init();

	/* Finish setting up lwp0 on our end before we call main() */
	riscv_init_lwp0_uarea();


	error = 0;
	if ((boothowto & RB_MD1) == 0) {
		VPRINTF("mpstart\n");
		if (plat->fp_mpstart)
			error = plat->fp_mpstart();
	}
	if (error)
		printf("AP startup problems\n");
}


#ifdef _LP64
static void
pte_bits(void (*pr)(const char *, ...), pt_entry_t pte)
{
	(*pr)("%c%c%c%c%c%c%c%c",
	    (pte & PTE_D) ? 'D' : '.',
	    (pte & PTE_A) ? 'A' : '.',
	    (pte & PTE_G) ? 'G' : '.',
	    (pte & PTE_U) ? 'U' : '.',
	    (pte & PTE_X) ? 'X' : '.',
	    (pte & PTE_W) ? 'W' : '.',
	    (pte & PTE_R) ? 'R' : '.',
	    (pte & PTE_V) ? 'V' : '.');
}

static void
dump_ln_table(paddr_t pdp_pa, int topbit, int level, vaddr_t va,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	pd_entry_t *pdp = (void *)PMAP_DIRECT_MAP(pdp_pa);

	(*pr)("l%u     @  pa %#16" PRIxREGISTER "\n", level, pdp_pa);
	for (size_t i = 0; i < PAGE_SIZE / sizeof(pd_entry_t); i++) {
		pd_entry_t entry = pdp[i];

		if (topbit) {
			va = i << (PGSHIFT + level * SEGLENGTH);
			if (va & __BIT(topbit)) {
				va |= __BITS(63, topbit);
			}
		}
		if (entry != 0) {
			paddr_t pa = __SHIFTOUT(entry, PTE_PPN) << PGSHIFT;
			// check level PPN bits.
			if (PTE_ISLEAF_P(entry)) {
				(*pr)("l%u %3zu    va 0x%016lx  pa 0x%012lx - ",
				      level, i, va, pa);
				pte_bits(pr, entry);
				(*pr)("\n");
			} else {
				(*pr)("l%u %3zu    va 0x%016lx  -> 0x%012lx - ",
				      level, i, va, pa);
				pte_bits(pr, entry);
				(*pr)("\n");
				if (level == 0) {
					(*pr)("wtf\n");
					continue;
				}
				if (pte_pde_valid_p(entry))
					dump_ln_table(pa, 0, level - 1, va, pr);
			}
		}
		va += 1UL << (PGSHIFT + level * SEGLENGTH);
	}
}

void
pt_dump(void (*pr)(const char *, ...) __printflike(1, 2))
{
	const register_t satp = csr_satp_read();
	size_t topbit = sizeof(long) * NBBY - 1;

#ifdef _LP64
	const paddr_t satp_pa = __SHIFTOUT(satp, SATP_PPN) << PGSHIFT;
	const uint8_t mode = __SHIFTOUT(satp, SATP_MODE);
	u_int level = 1;

	switch (mode) {
	case SATP_MODE_SV39:
	case SATP_MODE_SV48:
		topbit = (39 - 1) + (mode - 8) * SEGLENGTH;
		level = mode - 6;
		break;
	}
#endif
	(*pr)("topbit = %zu\n", topbit);

	(*pr)("satp   = 0x%" PRIxREGISTER "\n", satp);
#ifdef _LP64
	dump_ln_table(satp_pa, topbit, level, 0, pr);
#endif
}
#endif

void
consinit(void)
{
	static bool initialized = false;
	const struct fdt_console *cons = fdtbus_get_console();
	const struct fdt_platform *plat = fdt_platform_find();

	if (initialized || cons == NULL)
		return;

	u_int uart_freq = 0;
	extern struct bus_space riscv_generic_bs_tag;
	struct fdt_attach_args faa = {
		.faa_bst = &riscv_generic_bs_tag,
	};

	faa.faa_phandle = fdtbus_get_stdout_phandle();
	if (plat->fp_uart_freq != NULL)
		uart_freq = plat->fp_uart_freq();

	cons->consinit(&faa, uart_freq);

	initialized = true;
}
