/*	$NetBSD: riscv_machdep.c,v 1.18 2022/09/20 07:18:24 skrll Exp $	*/

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

#include "opt_modular.h"
#include "opt_riscv_debug.h"

#include <sys/cdefs.h>
__RCSID("$NetBSD: riscv_machdep.c,v 1.18 2022/09/20 07:18:24 skrll Exp $");

#include <sys/param.h>

#include <sys/boot_flag.h>
#include <sys/cpu.h>
#include <sys/exec.h>
#include <sys/kmem.h>
#include <sys/ktrace.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/systm.h>

#include <dev/cons.h>
#include <uvm/uvm_extern.h>

#include <riscv/locore.h>
#include <riscv/machdep.h>
#include <riscv/pte.h>

int cpu_printfataltraps;
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_memory.h>

#ifdef VERBOSE_INIT_RISCV
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

#ifndef FDT_MAX_BOOT_STRING
#define FDT_MAX_BOOT_STRING 1024
#endif

char bootargs[FDT_MAX_BOOT_STRING] = "";
char *boot_args = NULL;

static void
earlyconsputc(dev_t dev, int c)
{
	uartputc(c);
}

static int
earlyconsgetc(dev_t dev)
{
	return 0;
}

static struct consdev earlycons = {
	.cn_putc = earlyconsputc,
	.cn_getc = earlyconsgetc,
	.cn_pollc = nullcnpollc,
};

struct vm_map *phys_map;

struct trapframe cpu_ddb_regs;

struct cpu_info cpu_info_store = {
	.ci_cpl = IPL_HIGH,
	.ci_ddb_regs = &cpu_ddb_regs,
};

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

void
delay(unsigned long us)
{
	const uint32_t cycles_per_us = curcpu()->ci_data.cpu_cc_freq / 1000000;
	const uint64_t cycles = (uint64_t)us * cycles_per_us;
	const uint64_t finish = riscvreg_cycle_read() + cycles;

	while (riscvreg_cycle_read() < finish) {
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
 * Clear all registers except sp, pc, and t9.
 * $sp is set to the stack pointer passed in.  $pc is set to the entry
 * point given by the exec_package passed in, as is $t9 (used for PIC
 * code by the MIPS elf abi).
 */
void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;

	memset(tf, 0, sizeof(struct trapframe));
	tf->tf_sp = (intptr_t)stack_align(stack);
	tf->tf_pc = (intptr_t)pack->ep_entry & ~1;
#ifdef _LP64
	tf->tf_sr = (p->p_flag & PK_32) ? SR_USER32 : SR_USER;
#else
	tf->tf_sr = SR_USER;
#endif
	// Set up arguments for _start(obj, cleanup, ps_strings)
	tf->tf_a0 = 0;			// obj
	tf->tf_a1 = 0;			// cleanup
	tf->tf_a2 = p->p_psstrp;	// ps_strings
}

void
md_child_return(struct lwp *l)
{
	struct trapframe * const tf = l->l_md.md_utf;

	tf->tf_a0 = 0;
	tf->tf_a1 = 1;
#ifdef FPE
	tf->tf_sr &= ~SR_EF;		/* Disable FP as we can't be them. */
#endif
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

	kmem_free(uc, sizeof(ucontext_t));
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

	mcp->__private = (intptr_t)l->l_private;

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
		lwp_setprivate(l, (void *)(intptr_t)mcp->__private);
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

	return (0);
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
		l->l_md.md_astpending = 1;		/* force call to ast() */
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
		cpu_send_ipi(ci, IPI_AST);
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

void
cpu_reboot(int how, char *bootstr)
{
	for (;;) {
	}
}

void
cpu_dumpconf(void)
{
	// TBD!!
}

void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];	/* "99999 MB" */

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
}

static void
riscv_init_lwp0_uarea(void)
{
	extern char lwp0uspace[];

	uvm_lwp_setuarea(&lwp0, (vaddr_t)lwp0uspace);
	memset(&lwp0.l_md, 0, sizeof(lwp0.l_md));
	memset(lwp_getpcb(&lwp0), 0, sizeof(struct pcb));

	struct trapframe *tf = (struct trapframe *)(lwp0uspace + USPACE) - 1;
	memset(tf, 0, sizeof(struct trapframe));

	lwp0.l_md.md_utf = lwp0.l_md.md_ktf = tf;
}


static void
riscv_print_memory(const struct fdt_memory *m, void *arg)
{

	VPRINTF("FDT /memory @ 0x%" PRIx64 " size 0x%" PRIx64 "\n",
	    m->start, m->end - m->start);
}


static void
parse_bi_bootargs(char *args)
{
	int howto;

	for (char *cp = args; *cp; cp++) {
		/* Ignore superfluous '-', if there is one */
		if (*cp == '-')
			continue;

		howto = 0;
		BOOT_FLAG(*cp, howto);
		if (!howto)
			printf("bootflag '%c' not recognised\n", *cp);
		else
			boothowto |= howto;
	}
}


void
init_riscv(register_t hartid, vaddr_t vdtb)
{

	/* set temporally to work printf()/panic() even before consinit() */
	cn_tab = &earlycons;

	/* Load FDT */
	void *fdt_data = (void *)vdtb;
	int error = fdt_check_header(fdt_data);
	if (error != 0)
	    panic("fdt_check_header failed: %s", fdt_strerror(error));

	fdtbus_init(fdt_data);

#if 0
	/* Lookup platform specific backend */
	plat = riscv_fdt_platform();
	if (plat == NULL)
		panic("Kernel does not support this device");

#endif
	/* Early console may be available, announce ourselves. */
	VPRINTF("FDT<%p>\n", fdt_data);

	const int chosen = OF_finddevice("/chosen");
	if (chosen >= 0)
		OF_getprop(chosen, "bootargs", bootargs, sizeof(bootargs));
	boot_args = bootargs;

#if 0
	/*
	 * If stdout-path is specified on the command line, override the
	 * value in /chosen/stdout-path before initializing console.
	 */
	VPRINTF("stdout\n");
	fdt_update_stdout_path();
#endif

	/*
	 * Done making changes to the FDT.
	 */
	fdt_pack(fdt_data);

	VPRINTF("consinit ");
	consinit();
	VPRINTF("ok\n");

	/* Talk to the user */
	printf("NetBSD/riscv (fdt) booting ...\n");

#ifdef BOOT_ARGS
	char mi_bootargs[] = BOOT_ARGS;
	parse_bi_bootargs(mi_bootargs);
#endif

	/* SPAM me while testing */
	boothowto |= AB_DEBUG;

	uint64_t memory_start, memory_end;
	fdt_memory_get(&memory_start, &memory_end);

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

	/* Perform PT build and VM init */
	//cpu_kernel_vm_init();

	VPRINTF("bootargs: %s\n", bootargs);

	parse_bi_bootargs(boot_args);


	// initarm_common
	extern char __kernel_text[];
	extern char _end[];

	vaddr_t kernstart = trunc_page((vaddr_t)__kernel_text);
	vaddr_t kernend = round_page((vaddr_t)_end);
	paddr_t kernstart_phys __unused = KERN_VTOPHYS(kernstart);
	paddr_t kernend_phys __unused = KERN_VTOPHYS(kernend);

	vaddr_t kernelvmstart;

	vaddr_t kernstart_mega __unused = MEGAPAGE_TRUNC(kernstart);
	vaddr_t kernend_mega = MEGAPAGE_ROUND(kernend);

	kernelvmstart = kernend_mega;

#define DPRINTF(v)	VPRINTF("%24s = 0x%16lx\n", #v, (unsigned long)v);

	VPRINTF("------------------------------------------\n");
	DPRINTF(kern_vtopdiff);
	DPRINTF(memory_start);
	DPRINTF(memory_end);
	DPRINTF(memory_size);
	DPRINTF(kernstart_phys);
	DPRINTF(kernend_phys)
	DPRINTF(VM_MIN_KERNEL_ADDRESS);
	DPRINTF(kernstart_mega);
	DPRINTF(kernstart);
	DPRINTF(kernend);
	DPRINTF(kernend_mega);
	DPRINTF(VM_MAX_KERNEL_ADDRESS);
	VPRINTF("------------------------------------------\n");

#undef DPRINTF

	KASSERT(kernelvmstart < VM_KERNEL_VM_BASE);

	kernelvmstart = VM_KERNEL_VM_BASE;

	/*
	 * msgbuf is allocated from the bottom of any one of memory blocks
	 * to avoid corruption due to bootloader or changing kernel layout.
	 */
	paddr_t msgbufaddr = 0;

	KASSERT(msgbufaddr != 0);	/* no space for msgbuf */
#ifdef _LP64
	initmsgbuf((void *)RISCV_PA_TO_KVA(msgbufaddr), MSGBUFSIZE);
#endif

	uvm_md_init();

	pmap_bootstrap(kernelvmstart, VM_MAX_KERNEL_ADDRESS);

	/* Finish setting up lwp0 on our end before we call main() */
	riscv_init_lwp0_uarea();
}
