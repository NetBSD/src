/*	$NetBSD: arm32_machdep.c,v 1.90 2013/01/28 23:49:12 matt Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Machine dependent functions for kernel setup
 *
 * Created      : 17/09/94
 * Updated	: 18/04/01 updated for new wscons
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arm32_machdep.c,v 1.90 2013/01/28 23:49:12 matt Exp $");

#include "opt_modular.h"
#include "opt_md.h"
#include "opt_pmap_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/module.h>
#include <sys/atomic.h>
#include <sys/xcall.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>
#include <dev/mm.h>

#include <arm/arm32/katelib.h>
#include <arm/arm32/machdep.h>

#include <machine/bootconfig.h>
#include <machine/pcb.h>

void (*cpu_reset_address)(void);	/* Used by locore */
paddr_t cpu_reset_address_paddr;	/* Used by locore */

struct vm_map *phys_map = NULL;

#if defined(MEMORY_DISK_HOOKS) && !defined(MEMORY_DISK_ROOT_SIZE)
extern size_t md_root_size;		/* Memory disc size */
#endif	/* MEMORY_DISK_HOOKS && !MEMORY_DISK_ROOT_SIZE */

pv_addr_t kernelstack;
pv_addr_t abtstack;
pv_addr_t fiqstack;
pv_addr_t irqstack;
pv_addr_t undstack;
pv_addr_t idlestack;

void *	msgbufaddr;
extern paddr_t msgbufphys;

int kernel_debug = 0;
int cpu_fpu_present;

/* exported variable to be filled in by the bootloaders */
char *booted_kernel;

/* Prototypes */

void data_abort_handler(trapframe_t *frame);
void prefetch_abort_handler(trapframe_t *frame);
extern void configure(void);

/*
 * arm32_vector_init:
 *
 *	Initialize the vector page, and select whether or not to
 *	relocate the vectors.
 *
 *	NOTE: We expect the vector page to be mapped at its expected
 *	destination.
 */
void
arm32_vector_init(vaddr_t va, int which)
{
	if (CPU_IS_PRIMARY(curcpu())) {
		extern unsigned int page0[], page0_data[];
		unsigned int *vectors = (int *) va;
		unsigned int *vectors_data = vectors + (page0_data - page0);
		int vec;

		/*
		 * Loop through the vectors we're taking over, and copy the
		 * vector's insn and data word.
		 */
		for (vec = 0; vec < ARM_NVEC; vec++) {
			if ((which & (1 << vec)) == 0) {
				/* Don't want to take over this vector. */
				continue;
			}
			vectors[vec] = page0[vec];
			vectors_data[vec] = page0_data[vec];
		}

		/* Now sync the vectors. */
		cpu_icache_sync_range(va, (ARM_NVEC * 2) * sizeof(u_int));

		vector_page = va;
	}

	if (va == ARM_VECTORS_HIGH) {
		/*
		 * Assume the MD caller knows what it's doing here, and
		 * really does want the vector page relocated.
		 *
		 * Note: This has to be done here (and not just in
		 * cpu_setup()) because the vector page needs to be
		 * accessible *before* cpu_startup() is called.
		 * Think ddb(9) ...
		 *
		 * NOTE: If the CPU control register is not readable,
		 * this will totally fail!  We'll just assume that
		 * any system that has high vector support has a
		 * readable CPU control register, for now.  If we
		 * ever encounter one that does not, we'll have to
		 * rethink this.
		 */
		cpu_control(CPU_CONTROL_VECRELOC, CPU_CONTROL_VECRELOC);
	}
}

/*
 * Debug function just to park the CPU
 */

void
halt(void)
{
	while (1)
		cpu_sleep(0);
}


/* Sync the discs, unmount the filesystems, and adjust the todr */

void
bootsync(void)
{
	static bool bootsyncdone = false;

	if (bootsyncdone) return;

	bootsyncdone = true;

	/* Make sure we can still manage to do things */
	if (GetCPSR() & I32_bit) {
		/*
		 * If we get here then boot has been called without RB_NOSYNC
		 * and interrupts were disabled. This means the boot() call
		 * did not come from a user process e.g. shutdown, but must
		 * have come from somewhere in the kernel.
		 */
		IRQenable;
		printf("Warning IRQ's disabled during boot()\n");
	}

	vfs_shutdown();

	resettodr();
}

/*
 * void cpu_startup(void)
 *
 * Machine dependent startup code. 
 *
 */
void
cpu_startup(void)
{
	vaddr_t minaddr;
	vaddr_t maxaddr;
	u_int loop;
	char pbuf[9];

	/*
	 * Until we better locking, we have to live under the kernel lock.
	 */
	//KERNEL_LOCK(1, NULL);

	/* Set the CPU control register */
	cpu_setup(boot_args);

	/* Lock down zero page */
	vector_page_setprot(VM_PROT_READ);

	/*
	 * Give pmap a chance to set up a few more things now the vm
	 * is initialised
	 */
	pmap_postinit();

	/*
	 * Initialize error message buffer (at end of core).
	 */

	/* msgbufphys was setup during the secondary boot strap */
	for (loop = 0; loop < btoc(MSGBUFSIZE); ++loop)
		pmap_kenter_pa((vaddr_t)msgbufaddr + loop * PAGE_SIZE,
		    msgbufphys + loop * PAGE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));

	/*
	 * Identify ourselves for the msgbuf (everything printed earlier will
	 * not be buffered).
	 */
	printf("%s%s", copyright, version);

	format_bytes(pbuf, sizeof(pbuf), arm_ptob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	struct lwp * const l = &lwp0;
	struct pcb * const pcb = lwp_getpcb(l);
	pcb->pcb_ksp = uvm_lwp_getuarea(l) + USPACE_SVC_STACK_TOP;
	lwp_settrapframe(l, (struct trapframe *)pcb->pcb_ksp - 1);
}

/*
 * machine dependent system variables.
 */
static int
sysctl_machdep_booted_device(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	if (booted_device == NULL)
		return (EOPNOTSUPP);

	node = *rnode;
	node.sysctl_data = __UNCONST(device_xname(booted_device));
	node.sysctl_size = strlen(device_xname(booted_device)) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

static int
sysctl_machdep_booted_kernel(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	if (booted_kernel == NULL || booted_kernel[0] == '\0')
		return (EOPNOTSUPP);

	node = *rnode;
	node.sysctl_data = booted_kernel;
	node.sysctl_size = strlen(booted_kernel) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

static int
sysctl_machdep_powersave(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	int error, newval;

	newval = cpu_do_powersave;
	node.sysctl_data = &newval;
	if (cpufuncs.cf_sleep == (void *) cpufunc_nullop)
		node.sysctl_flags &= ~CTLFLAG_READWRITE;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL || newval == cpu_do_powersave)
		return (error);

	if (newval < 0 || newval > 1)
		return (EINVAL);
	cpu_do_powersave = newval;

	return (0);
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
		       CTLTYPE_INT, "debug", NULL,
		       NULL, 0, &kernel_debug, 0,
		       CTL_MACHDEP, CPU_DEBUG, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_device", NULL,
		       sysctl_machdep_booted_device, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_DEVICE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_booted_kernel, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "powersave", NULL,
		       sysctl_machdep_powersave, 0, &cpu_do_powersave, 0,
		       CTL_MACHDEP, CPU_POWERSAVE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "fpu_present", NULL,
		       NULL, 0, &cpu_fpu_present, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}

void
parse_mi_bootargs(char *args)
{
	int integer;

	if (get_bootconf_option(args, "single", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-s", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_SINGLE;
	if (get_bootconf_option(args, "kdb", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-k", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-d", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_KDB;
	if (get_bootconf_option(args, "ask", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-a", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_ASKNAME;

#ifdef PMAP_DEBUG
	if (get_bootconf_option(args, "pmapdebug", BOOTOPT_TYPE_INT, &integer)) {
		pmap_debug_level = integer;
		pmap_debug(pmap_debug_level);
	}
#endif	/* PMAP_DEBUG */

/*	if (get_bootconf_option(args, "nbuf", BOOTOPT_TYPE_INT, &integer))
		bufpages = integer;*/

#if defined(MEMORY_DISK_HOOKS) && !defined(MEMORY_DISK_ROOT_SIZE)
	if (get_bootconf_option(args, "memorydisc", BOOTOPT_TYPE_INT, &integer)
	    || get_bootconf_option(args, "memorydisk", BOOTOPT_TYPE_INT, &integer)) {
		md_root_size = integer;
		md_root_size *= 1024;
		if (md_root_size < 32*1024)
			md_root_size = 32*1024;
		if (md_root_size > 2048*1024)
			md_root_size = 2048*1024;
	}
#endif	/* MEMORY_DISK_HOOKS && !MEMORY_DISK_ROOT_SIZE */

	if (get_bootconf_option(args, "quiet", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-q", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= AB_QUIET;
	if (get_bootconf_option(args, "verbose", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-v", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= AB_VERBOSE;
}

#ifdef __HAVE_FAST_SOFTINTS
#if IPL_SOFTSERIAL != IPL_SOFTNET + 1
#error IPLs are screwed up
#elif IPL_SOFTNET != IPL_SOFTBIO + 1
#error IPLs are screwed up
#elif IPL_SOFTBIO != IPL_SOFTCLOCK + 1
#error IPLs are screwed up
#elif !(IPL_SOFTCLOCK > IPL_NONE)
#error IPLs are screwed up
#elif (IPL_NONE != 0)
#error IPLs are screwed up
#endif

#ifndef __HAVE_PIC_FAST_SOFTINTS
#define	SOFTINT2IPLMAP \
	(((IPL_SOFTSERIAL - IPL_SOFTCLOCK) << (SOFTINT_SERIAL * 4)) | \
	 ((IPL_SOFTNET    - IPL_SOFTCLOCK) << (SOFTINT_NET    * 4)) | \
	 ((IPL_SOFTBIO    - IPL_SOFTCLOCK) << (SOFTINT_BIO    * 4)) | \
	 ((IPL_SOFTCLOCK  - IPL_SOFTCLOCK) << (SOFTINT_CLOCK  * 4)))
#define	SOFTINT2IPL(l)	((SOFTINT2IPLMAP >> ((l) * 4)) & 0x0f)

/*
 * This returns a mask of softint IPLs that be dispatch at <ipl>
 * SOFTIPLMASK(IPL_NONE)	= 0x0000000f
 * SOFTIPLMASK(IPL_SOFTCLOCK)	= 0x0000000e
 * SOFTIPLMASK(IPL_SOFTBIO)	= 0x0000000c
 * SOFTIPLMASK(IPL_SOFTNET)	= 0x00000008
 * SOFTIPLMASK(IPL_SOFTSERIAL)	= 0x00000000
 */
#define	SOFTIPLMASK(ipl) ((0x0f << (ipl)) & 0x0f)

void softint_switch(lwp_t *, int);

void
softint_trigger(uintptr_t mask)
{
	curcpu()->ci_softints |= mask;
}

void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{
	lwp_t ** lp = &l->l_cpu->ci_softlwps[level];
	KASSERT(*lp == NULL || *lp == l);
	*lp = l;
	*machdep = 1 << SOFTINT2IPL(level);
	KASSERT(level != SOFTINT_CLOCK || *machdep == (1 << (IPL_SOFTCLOCK - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_BIO || *machdep == (1 << (IPL_SOFTBIO - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_NET || *machdep == (1 << (IPL_SOFTNET - IPL_SOFTCLOCK)));
	KASSERT(level != SOFTINT_SERIAL || *machdep == (1 << (IPL_SOFTSERIAL - IPL_SOFTCLOCK)));
}

void
dosoftints(void)
{
	struct cpu_info * const ci = curcpu();
	const int opl = ci->ci_cpl;
	const uint32_t softiplmask = SOFTIPLMASK(opl);

	splhigh();
	for (;;) {
		u_int softints = ci->ci_softints & softiplmask;
		KASSERT((softints != 0) == ((ci->ci_softints >> opl) != 0));
		KASSERT(opl == IPL_NONE || (softints & (1 << (opl - IPL_SOFTCLOCK))) == 0);
		if (softints == 0) {
			splx(opl);
			return;
		}
#define	DOSOFTINT(n) \
		if (ci->ci_softints & (1 << (IPL_SOFT ## n - IPL_SOFTCLOCK))) { \
			ci->ci_softints &= \
			    ~(1 << (IPL_SOFT ## n - IPL_SOFTCLOCK)); \
			softint_switch(ci->ci_softlwps[SOFTINT_ ## n], \
			    IPL_SOFT ## n); \
			continue; \
		}
		DOSOFTINT(SERIAL);
		DOSOFTINT(NET);
		DOSOFTINT(BIO);
		DOSOFTINT(CLOCK);
		panic("dosoftints wtf (softints=%u?, ipl=%d)", softints, opl);
	}
}
#endif /* !__HAVE_PIC_FAST_SOFTINTS */
#endif /* __HAVE_FAST_SOFTINTS */

#ifdef MODULAR
/*
 * Push any modules loaded by the boot loader.
 */
void
module_init_md(void)
{
}
#endif /* MODULAR */

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (pa < ctob(physmem)) ? 0 : EFAULT;
}

#ifdef __HAVE_CPU_UAREA_ALLOC_IDLELWP
vaddr_t
cpu_uarea_alloc_idlelwp(struct cpu_info *ci)
{
	const vaddr_t va = idlestack.pv_va + ci->ci_cpuid * USPACE;
	// printf("%s: %s: va=%lx\n", __func__, ci->ci_data.cpu_name, va);
	return va;
}
#endif

#ifdef MULTIPROCESSOR
void
cpu_boot_secondary_processors(void)
{
	uint32_t mbox;
	kcpuset_export_u32(kcpuset_attached, &mbox, sizeof(mbox));
	atomic_swap_32(&arm_cpu_mbox, mbox);
	membar_producer();
#ifdef _ARM_ARCH_7
	__asm __volatile("sev; sev; sev");
#endif
}

void
xc_send_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);


	if (ci) {
		/* Unicast, remote CPU */
		printf("%s: -> %s", __func__, ci->ci_data.cpu_name);
		intr_ipi_send(ci->ci_kcpuset, IPI_XCALL);
	} else {
		printf("%s: -> !%s", __func__, ci->ci_data.cpu_name);
		/* Broadcast to all but ourselves */
		kcpuset_t *kcp;
		kcpuset_create(&kcp, (ci != NULL));
		KASSERT(kcp != NULL);
		kcpuset_copy(kcp, kcpuset_running);
		kcpuset_clear(kcp, cpu_index(ci));
		intr_ipi_send(kcp, IPI_XCALL);
		kcpuset_destroy(kcp);
	}
	printf("\n");
}
#endif /* MULTIPROCESSOR */

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
bool
mm_md_direct_mapped_phys(paddr_t pa, vaddr_t *vap)
{
	if (physical_start <= pa && pa < physical_end) {
		*vap = KERNEL_BASE + (pa - physical_start);
		return true;
	}

	return false;
}
#endif
