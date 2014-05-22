/* $NetBSD: cpu.c,v 1.28.12.1 2014/05/22 11:39:25 yamt Exp $ */

/*-
 * Copyright (c) 2000, 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * cpu.c - high-level CPU detection etc
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.28.12.1 2014/05/22 11:39:25 yamt Exp $");

#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <uvm/uvm_extern.h>
#include <arm/armreg.h>
#include <arm/cpuconf.h>
#include <arm/undefined.h>
#include <machine/machdep.h>
#include <machine/pcb.h>

#include <arch/acorn26/acorn26/cpuvar.h>

static int cpu_match(device_t, cfdata_t, void *);
static void cpu_attach(device_t, device_t, void *);
static int cpu_search(device_t, cfdata_t, const int *, void *);
static register_t cpu_identify(void);
#ifdef CPU_ARM2
static int arm2_undef_handler(u_int, u_int, struct trapframe *, int);
static int swp_handler(u_int, u_int, struct trapframe *, int);
#endif
#ifdef CPU_ARM3
static void cpu_arm3_setup(device_t, int);
#endif
static void cpu_delay_calibrate(device_t);

CFATTACH_DECL_NEW(cpu_root, 0, cpu_match, cpu_attach, NULL, NULL);

/* cf_flags bits */
#define CFF_NOCACHE	0x00000001

static int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{

	if (curcpu()->ci_dev == NULL)
		return 1;
	return 0;
}

static void
cpu_attach(device_t parent, device_t self, void *aux)
{
	int supported;

	curcpu()->ci_dev = self;
	aprint_normal(": ");
	curcpu()->ci_arm_cpuid = cpu_identify();
	cputype = curcpu()->ci_arm_cputype =
	    curcpu()->ci_arm_cpuid & CPU_ID_CPU_MASK;
	curcpu()->ci_arm_cpurev =
	    curcpu()->ci_arm_cpuid & CPU_ID_REVISION_MASK;

	supported = 0;
	switch (curcpu()->ci_arm_cputype) {
	case CPU_ID_ARM2:
		aprint_normal("ARM2");
#ifdef CPU_ARM2
		supported = 1;
		install_coproc_handler(CORE_UNKNOWN_HANDLER,
		    arm2_undef_handler);
#endif
		break;
	case CPU_ID_ARM250:
		aprint_normal("ARM250");
#ifdef CPU_ARM250
		supported = 1;
#endif
		break;
	case CPU_ID_ARM3:
		aprint_normal("ARM3 (rev. %u)", curcpu()->ci_arm_cpurev);
#ifdef CPU_ARM3
		supported = 1;
		cpu_arm3_setup(self, device_cfdata(self)->cf_flags);
#endif
		break;
	default:
		aprint_normal("Unknown type, ID=0x%08x",
		    curcpu()->ci_arm_cputype);
		break;
	}
	aprint_normal("\n");
	set_cpufuncs();
	if (!supported)
		aprint_error_dev(self,
		    "WARNING: CPU type not supported by kernel\n");
	config_interrupts(self, cpu_delay_calibrate);
	config_search_ia(cpu_search, self, "cpu", NULL);
}

static int
cpu_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	
	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, NULL);

	return 0;
}

static label_t undef_jmp;

static int
cpu_undef_handler(u_int addr, u_int insn, struct trapframe *tf, int fault_code)
{

	longjmp(&undef_jmp);
}

static register_t
cpu_identify(void)
{
	register_t dummy;
	volatile register_t id = CPU_ID_ARM2;
	void *cp_core, *cp15;

	cp_core = install_coproc_handler(CORE_UNKNOWN_HANDLER,
	    cpu_undef_handler);
	cp15 = install_coproc_handler(SYSTEM_COPROC, cpu_undef_handler);
	if (setjmp(&undef_jmp) == 0) {
		/* ARM250 and ARM3 support SWP. */
		__asm volatile ("swp r0, r0, [%0]" : : "r" (&dummy) : "r0");
		id = CPU_ID_ARM250;
		/* ARM3 has an internal coprocessor 15 with an ID register. */
		__asm volatile ("mrc 15, 0, %0, cr0, cr0" : "=r" (id));
	}
	remove_coproc_handler(cp_core);
	remove_coproc_handler(cp15);
	return id;
}

#ifdef CPU_ARM2
static int
arm2_undef_handler(u_int addr, u_int insn, struct trapframe *frame,
    int fault_code)
{

	if ((insn & 0x0fb00ff0) == 0x01000090)
		/* It's a SWP */
		return swp_handler(addr, insn, frame, fault_code);
	/*
	 * Check if the aborted instruction was a SWI (ARM2 bug --
	 * ARM3 data sheet p87) and call SWI handler if so.
	 */
	if ((insn & 0x0f000000) == 0x0f000000) {
		swi_handler(frame);
		return 0;
	}
	return 1;
}

/*
 * In order for the following macro to work, any function using it
 * must ensure that tf->r15 is copied into getreg(15).  This is safe
 * with the current trapframe layout on acorn26, but be careful.
 */
#define getreg(r) (((register_t *)&tf->tf_r0)[r])

static int
swp_handler(u_int addr, u_int insn, struct trapframe *tf, int fault_code)
{
	struct proc *p = curlwp->l_proc;
	int rd, rm, rn, byte;
	register_t temp;
	void *uaddr;
	int err;
	
	KASSERT(fault_code & FAULT_USER);
	rd = (insn & 0x0000f000) >> 12;
	rm = (insn & 0x0000000f);
	rn = (insn & 0x000f0000) >> 16;
	byte = insn & 0x00400000;

	if (rd == 15 || rm == 15 || rn == 15)
		/* UNPREDICTABLE.  Arbitrarily do nothing. */
		return 0;
	uaddr = (void *)getreg(rn);
	/* We want the page wired so we won't sleep */
	/* XXX only wire one byte due to weirdness with unaligned words */
	err = uvm_vslock(p->p_vmspace, uaddr, 1, VM_PROT_READ | VM_PROT_WRITE);
	if (err != 0) {
		ksiginfo_t ksi;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_addr = uaddr;
		ksi.ksi_code = SEGV_MAPERR;
		trapsignal(curlwp, &ksi);
		return 0;
	}
	/* I believe the uvm_vslock() guarantees the fetch/store won't fail. */
	if (byte) {
		temp = fubyte(uaddr);
		subyte(uaddr, getreg(rm));
		getreg(rd) = temp;
	} else {
		/*
		 * XXX Unaligned addresses happen to be handled
		 * appropriately by [fs]uword at present.
		 */
		temp = fuword(uaddr);
		suword(uaddr, getreg(rm));
		getreg(rd) = temp;
	}
	uvm_vsunlock(p->p_vmspace, uaddr, 1);
	return 0;
}
#endif

#ifdef CPU_ARM3

#define ARM3_READ(reg, var) \
	__asm ("mrc 15, 0, %0, cr" __STRING(reg) ", cr0" : "=r" (var))
#define ARM3_WRITE(reg, val) \
	__asm ("mcr 15, 0, %0, cr" __STRING(reg) ", cr0" : : "r" (val))

static void
cpu_arm3_setup(device_t self, int flags)
{

	/* Disable the cache while we set things up. */
	ARM3_WRITE(ARM3_CP15_CONTROL, ARM3_CTL_SHARED);
	if (flags & CFF_NOCACHE) {
		aprint_normal(", cache disabled");
		return;
	}
	/* All RAM and ROM is cacheable. */
	ARM3_WRITE(ARM3_CP15_CACHEABLE, 0xfcffffff);
	/* All RAM is updateable. */
	ARM3_WRITE(ARM3_CP15_UPDATEABLE, 0x00ffffff);
	/* Nothing is disruptive.  We'll do cache flushing manually. */
	ARM3_WRITE(ARM3_CP15_DISRUPTIVE, 0x00000000);
	/* Flush the cache and turn it on. */
	ARM3_WRITE(ARM3_CP15_FLUSH, 0);
	ARM3_WRITE(ARM3_CP15_CONTROL, ARM3_CTL_CACHE_ON | ARM3_CTL_SHARED);
	aprint_normal(", cache enabled");
	cpu_delay_factor = 8;
}
#endif

/* XXX This should be inlined. */
void
cpu_cache_flush(void)
{

#ifdef CPU_ARM3
#if defined(CPU_ARM2) || defined(CPU_ARM250)
	if ((cputype & CPU_ID_CPU_MASK) == CPU_ID_ARM3)
#endif
		ARM3_WRITE(ARM3_CP15_FLUSH, 0);
#endif
}

int cpu_delay_factor = 1;

static void
cpu_delay_calibrate(device_t self)
{
	struct timeval startt, end, diff;

	microtime(&startt);
	cpu_delayloop(10000);
	microtime(&end);
	timersub(&end, &startt, &diff);
	cpu_delay_factor = 10000 / diff.tv_usec + 1;
	aprint_normal_dev(self, "10000 loops in %d microseconds, "
	    "delay factor = %d\n",
	    diff.tv_usec, cpu_delay_factor);
}
