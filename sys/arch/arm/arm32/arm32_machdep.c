/*	$NetBSD: arm32_machdep.c,v 1.145 2023/01/19 08:03:51 mlelstv Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: arm32_machdep.c,v 1.145 2023/01/19 08:03:51 mlelstv Exp $");

#include "opt_arm_debug.h"
#include "opt_arm_start.h"
#include "opt_fdt.h"
#include "opt_modular.h"
#include "opt_md.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>

#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/ipi.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/xcall.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>
#include <dev/mm.h>

#include <arm/locore.h>

#include <arm/cpu_topology.h>
#include <arm/arm32/machdep.h>

#include <machine/bootconfig.h>
#include <machine/pcb.h>

#if defined(FDT)
#include <arm/fdt/arm_fdtvar.h>
#include <arch/evbarm/fdt/platform.h>
#endif

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#ifdef __HAVE_GENERIC_START
void generic_prints(const char *);
void generic_printx(int);
#define VPRINTS(s)	generic_prints(s)
#define VPRINTX(x)	generic_printx(x)
#else
#define VPRINTS(s)	__nothing
#define VPRINTX(x)	__nothing
#endif
#else
#define VPRINTF(...)	__nothing
#define VPRINTS(s)	__nothing
#define VPRINTX(x)	__nothing
#endif

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
int cpu_printfataltraps = 0;
int cpu_fpu_present;
int cpu_hwdiv_present;
int cpu_neon_present;
int cpu_simd_present;
int cpu_simdex_present;
int cpu_umull_present;
int cpu_synchprim_present;
int cpu_unaligned_sigbus;
const char *cpu_arch = "";

int cpu_instruction_set_attributes[6];
int cpu_memory_model_features[4];
int cpu_processor_features[2];
int cpu_media_and_vfp_features[2];

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
#if defined(CPU_ARMV7) || defined(CPU_ARM11) || defined(ARM_HAS_VBAR)
	/*
	 * If this processor has the security extension, don't bother
	 * to move/map the vector page.  Simply point VBAR to the copy
	 * that exists in the .text segment.
	 */
#ifndef ARM_HAS_VBAR
	if (va == ARM_VECTORS_LOW
	    && (armreg_pfr1_read() & ARM_PFR1_SEC_MASK) != 0) {
#endif
		extern const uint32_t page0rel[];
		vector_page = (vaddr_t)page0rel;
		KASSERT((vector_page & 0x1f) == 0);
		armreg_vbar_write(vector_page);
		VPRINTF(" vbar=%p", page0rel);
		cpu_control(CPU_CONTROL_VECRELOC, 0);
		return;
#ifndef ARM_HAS_VBAR
	}
#endif
#endif
#ifndef ARM_HAS_VBAR
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
#endif
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

#ifndef __HAVE_GENERIC_START
	/* Set the CPU control register */
	cpu_setup(boot_args);
#endif

#ifndef ARM_HAS_VBAR
	/* Lock down zero page */
	vector_page_setprot(VM_PROT_READ);
#endif

	/*
	 * Give pmap a chance to set up a few more things now the vm
	 * is initialised
	 */
	pmap_postinit();

#ifdef FDT
	const struct arm_platform * const plat = arm_fdt_platform();
	if (plat->ap_startup != NULL)
		plat->ap_startup();
#endif

	/*
	 * Initialize error message buffer (at end of core).
	 */

	/* msgbufphys was setup during the secondary boot strap */
	if (!pmap_extract(pmap_kernel(), (vaddr_t)msgbufaddr, NULL)) {
		for (u_int loop = 0; loop < btoc(MSGBUFSIZE); ++loop) {
			pmap_kenter_pa((vaddr_t)msgbufaddr + loop * PAGE_SIZE,
			    msgbufphys + loop * PAGE_SIZE,
			    VM_PROT_READ|VM_PROT_WRITE, 0);
		}
	}
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));

	/*
	 * Allocate a submap for physio
	 */
	minaddr = 0;
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);

	banner();

	/*
	 * This is actually done by initarm_common, but not all ports use it
	 * yet so do it here to catch them as well
	 */
	struct lwp * const l = &lwp0;
	struct pcb * const pcb = lwp_getpcb(l);

	/* Zero out the PCB. */
 	memset(pcb, 0, sizeof(*pcb));

	pcb->pcb_ksp = uvm_lwp_getuarea(l) + USPACE_SVC_STACK_TOP;
	pcb->pcb_ksp -= sizeof(struct trapframe);

	struct trapframe * tf = (struct trapframe *)pcb->pcb_ksp;

	/* Zero out the trapframe. */
	memset(tf, 0, sizeof(*tf));
	lwp_settrapframe(l, tf);

 	tf->tf_spsr = PSR_USR32_MODE;
#ifdef _ARM_ARCH_BE8
	tf->tf_spsr |= PSR_E_BIT;
#endif

	cpu_startup_hook();
}

__weak_alias(cpu_startup_hook,cpu_startup_default)
void
cpu_startup_default(void)
{
}

/*
 * machine dependent system variables.
 */
static int
sysctl_machdep_booted_device(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	if (booted_device == NULL)
		return EOPNOTSUPP;

	node = *rnode;
	node.sysctl_data = __UNCONST(device_xname(booted_device));
	node.sysctl_size = strlen(device_xname(booted_device)) + 1;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
sysctl_machdep_booted_kernel(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	if (booted_kernel == NULL || booted_kernel[0] == '\0')
		return EOPNOTSUPP;

	node = *rnode;
	node.sysctl_data = booted_kernel;
	node.sysctl_size = strlen(booted_kernel) + 1;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
sysctl_machdep_cpu_arch(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	node.sysctl_data = __UNCONST(cpu_arch);
	node.sysctl_size = strlen(cpu_arch) + 1;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
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
		return error;

	if (newval < 0 || newval > 1)
		return EINVAL;
	cpu_do_powersave = newval;

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
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "cpu_arch", NULL,
		       sysctl_machdep_cpu_arch, 0, NULL, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "powersave", NULL,
		       sysctl_machdep_powersave, 0, &cpu_do_powersave, 0,
		       CTL_MACHDEP, CPU_POWERSAVE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "cpu_id", NULL,
		       NULL, curcpu()->ci_arm_cpuid, NULL, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
#ifdef FPU_VFP
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "fpu_id", NULL,
		       NULL, 0, &cpu_info_store[0].ci_vfp_id, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "fpu_present", NULL,
		       NULL, 0, &cpu_fpu_present, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "hwdiv_present", NULL,
		       NULL, 0, &cpu_hwdiv_present, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "neon_present", NULL,
		       NULL, 0, &cpu_neon_present, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "id_isar", NULL,
		       NULL, 0,
		       cpu_instruction_set_attributes,
		       sizeof(cpu_instruction_set_attributes),
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "id_mmfr", NULL,
		       NULL, 0,
		       cpu_memory_model_features,
		       sizeof(cpu_memory_model_features),
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "id_pfr", NULL,
		       NULL, 0,
		       cpu_processor_features,
		       sizeof(cpu_processor_features),
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "id_mvfr", NULL,
		       NULL, 0,
		       cpu_media_and_vfp_features,
		       sizeof(cpu_media_and_vfp_features),
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "simd_present", NULL,
		       NULL, 0, &cpu_simd_present, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "simdex_present", NULL,
		       NULL, 0, &cpu_simdex_present, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "synchprim_present", NULL,
		       NULL, 0, &cpu_synchprim_present, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "printfataltraps", NULL,
		       NULL, 0, &cpu_printfataltraps, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	cpu_unaligned_sigbus =
#if defined(__ARMEL__)
	    !CPU_IS_ARMV6_P() && !CPU_IS_ARMV7_P();
#elif defined(_ARM_ARCH_BE8)
	    0;
#else
	    1;
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "unaligned_sigbus",
		       SYSCTL_DESCR("Do SIGBUS for fixed unaligned accesses"),
		       NULL, 0, &cpu_unaligned_sigbus, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}

void
parse_mi_bootargs(char *args)
{
	int integer;

	if (get_bootconf_option(args, "-1", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_MD1;
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
	if (get_bootconf_option(args, "userconf", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-c", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_USERCONF;
	if (get_bootconf_option(args, "halt", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-b", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_HALT;
	if (get_bootconf_option(args, "-1", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_MD1;
	if (get_bootconf_option(args, "-2", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_MD2;
	if (get_bootconf_option(args, "-3", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_MD3;
	if (get_bootconf_option(args, "-4", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= RB_MD4;

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
	if (get_bootconf_option(args, "debug", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-x", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= AB_DEBUG;
	if (get_bootconf_option(args, "silent", BOOTOPT_TYPE_BOOLEAN, &integer)
	    || get_bootconf_option(args, "-z", BOOTOPT_TYPE_BOOLEAN, &integer))
		if (integer)
			boothowto |= AB_SILENT;
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
	int s;

	s = splhigh();
	KASSERT(s == opl);
	for (;;) {
		u_int softints = ci->ci_softints & softiplmask;
		KASSERT((softints != 0) == ((ci->ci_softints >> opl) != 0));
		KASSERT(opl == IPL_NONE || (softints & (1 << (opl - IPL_SOFTCLOCK))) == 0);
		if (softints == 0) {
			break;
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
	splx(s);
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
#ifdef FDT
	arm_fdt_module_init();
#endif
}
#endif /* MODULAR */

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{
	if (pa >= physical_start && pa < physical_end)
		return 0;

	return kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL);
}

#ifdef __HAVE_CPU_UAREA_ALLOC_IDLELWP
vaddr_t
cpu_uarea_alloc_idlelwp(struct cpu_info *ci)
{
	const vaddr_t va = idlestack.pv_va + cpu_index(ci) * USPACE;
	// printf("%s: %s: va=%lx\n", __func__, ci->ci_data.cpu_name, va);
	return va;
}
#endif

#ifdef MULTIPROCESSOR
/*
 * Initialise a secondary processor.
 *
 * printf isn't available to us for a number of reasons.
 *
 * -  kprint_init has been called and printf will try to take locks which we
 *    can't do just yet because bootstrap translation tables do not allowing
 *    caching.
 *
 * -  kmutex(9) relies on curcpu which isn't setup yet.
 *
 */
void __noasan
cpu_init_secondary_processor(int cpuindex)
{
	// pmap_kernel has been successfully built and we can switch to it
	cpu_domains(DOMAIN_DEFAULT);
	cpu_idcache_wbinv_all();

	VPRINTS("index: ");
	VPRINTX(cpuindex);
	VPRINTS(" ttb");

	cpu_setup(boot_args);

#ifdef ARM_MMU_EXTENDED
	/*
	 * TTBCR should have been initialized by the MD start code.
	 */
	KASSERT((armreg_contextidr_read() & 0xff) == 0);
	KASSERT(armreg_ttbcr_read() == __SHIFTIN(1, TTBCR_S_N));
	/*
	 * Disable lookups via TTBR0 until there is an activated pmap.
	 */

	armreg_ttbcr_write(armreg_ttbcr_read() | TTBCR_S_PD0);
	cpu_setttb(pmap_kernel()->pm_l1_pa , KERNEL_PID);
	isb();
#else
	cpu_setttb(pmap_kernel()->pm_l1->l1_physaddr, true);
#endif

	cpu_tlb_flushID();

	VPRINTS(" (TTBR0=");
	VPRINTX(armreg_ttbr_read());
	VPRINTS(")");

#ifdef ARM_MMU_EXTENDED
	VPRINTS(" (TTBR1=");
	VPRINTX(armreg_ttbr1_read());
	VPRINTS(")");
	VPRINTS(" (TTBCR=");
	VPRINTX(armreg_ttbcr_read());
	VPRINTS(")");
#endif

	struct cpu_info * ci = &cpu_info_store[cpuindex];

	VPRINTS(" ci = ");
	VPRINTX((int)ci);

	ci->ci_ctrl = armreg_sctlr_read();
	ci->ci_arm_cpuid = cpu_idnum();
	ci->ci_arm_cputype = ci->ci_arm_cpuid & CPU_ID_CPU_MASK;
	ci->ci_arm_cpurev = ci->ci_arm_cpuid & CPU_ID_REVISION_MASK;

	ci->ci_midr = armreg_midr_read();
	ci->ci_actlr = armreg_auxctl_read();
	ci->ci_revidr = armreg_revidr_read();
	ci->ci_mpidr = armreg_mpidr_read();

	arm_cpu_topology_set(ci, ci->ci_mpidr);

	VPRINTS(" vfp");
	vfp_detect(ci);

	VPRINTS(" hatched |=");
	VPRINTX(__BIT(cpuindex));
	VPRINTS("\n\r");

	cpu_set_hatched(cpuindex);

	/*
	 * return to assembly to wait for cpu_boot_secondary_processors
	 */
}

void
xc_send_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	intr_ipi_send(ci != NULL ? ci->ci_kcpuset : NULL, IPI_XCALL);
}

void
cpu_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	intr_ipi_send(ci != NULL ? ci->ci_kcpuset : NULL, IPI_GENERIC);
}

#endif /* MULTIPROCESSOR */

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
bool
mm_md_direct_mapped_phys(paddr_t pa, vaddr_t *vap)
{
	bool rv;
	vaddr_t va = pmap_direct_mapped_phys(pa, &rv, 0);
	if (rv) {
		*vap = va;
	}
	return rv;
}
#endif

bool
mm_md_page_color(paddr_t pa, int *colorp)
{
#if (ARM_MMU_V6 + ARM_MMU_V7) != 0
	*colorp = atop(pa & arm_cache_prefer_mask);

	return arm_cache_prefer_mask ? false : true;
#else
	*colorp = 0;

	return true;
#endif
}

#if defined(FDT)
extern char KERNEL_BASE_phys[];
#define KERNEL_BASE_PHYS ((paddr_t)KERNEL_BASE_phys)

void
cpu_kernel_vm_init(paddr_t memory_start, psize_t memory_size)
{
	const struct arm_platform *plat = arm_fdt_platform();

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	const bool mapallmem_p = true;
#ifndef PMAP_NEED_ALLOC_POOLPAGE
	if (memory_size > KERNEL_VM_BASE - KERNEL_BASE) {
		VPRINTF("%s: dropping RAM size from %luMB to %uMB\n",
		    __func__, (unsigned long) (memory_size >> 20),
		    (KERNEL_VM_BASE - KERNEL_BASE) >> 20);
		memory_size = KERNEL_VM_BASE - KERNEL_BASE;
	}
#endif
#else
	const bool mapallmem_p = false;
#endif

	VPRINTF("%s: kernel phys start %" PRIxPADDR " end %" PRIxPADDR "\n",
	    __func__, memory_start, memory_start + memory_size);

	arm32_bootmem_init(memory_start, memory_size, KERNEL_BASE_PHYS);
	arm32_kernel_vm_init(KERNEL_VM_BASE, ARM_VECTORS_HIGH, 0,
	    plat->ap_devmap(), mapallmem_p);
}
#endif

