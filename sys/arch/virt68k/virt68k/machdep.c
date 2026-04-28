/*	$NetBSD: machdep.c,v 1.43 2026/04/28 03:29:11 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.43 2026/04/28 03:29:11 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_m68k_arch.h"

#define	_M68K_BUS_SPACE_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/boot_flag.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <sys/exec_elf.h>
#endif

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/vmparam.h>
#include <m68k/cacheops.h>
#include <m68k/linux_bootinfo.h>
#include <m68k/seglist.h>
#include <dev/cons.h>
#include <dev/mm.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#endif

#include "gftty.h"
#if NGFTTY > 0
#include <dev/goldfish/gfttyvar.h>
#endif  

/* Machine-dependent initialization routines. */
void	machine_init(paddr_t);

/*
 * Clamp the kernel virtual address space to keep it out of the
 * TT ranges we use for devices.
 */
struct pmap_bootmap machine_bootmap[] = {
	{ .pmbm_vaddr = VIRT68K_IO_BASE,
	  .pmbm_size  = VIRT68K_IO_SIZE,
	  .pmbm_flags = PMBM_F_KEEPOUT },

	{ .pmbm_vaddr = -1 },
};

static void
machine_cnattach(void (*func)(bus_space_tag_t, bus_space_handle_t),
    paddr_t addr, paddr_t size)
{
	extern paddr_t consdev_addr;
	bus_space_tag_t bst = &m68k_simple_bus_space;
	bus_space_handle_t bsh;

	if (bus_space_map(bst, addr, size, 0, &bsh) == 0) {
		func(bst, bsh);
	}
	consdev_addr = addr;
}

/*
 * Early initialization, right before main is called.
 */
void
machine_init(paddr_t nextpa)
{
	struct bi_record *bi __unused;

	extern paddr_t msgbufpa;

	/*
	 * Find the console in the bootinfo and attach it.
	 */
#if NGFTTY > 0
	bi = bootinfo_find(BI_VIRT_GF_TTY_BASE);
	if (bi != NULL) {
		struct bi_virt_dev *vd = bootinfo_dataptr(bi);
		machine_cnattach(gftty_cnattach, vd->vd_mmio_base, 0x1000);
		printf("Initialized Goldfish TTY console @ 0x%08x\n",
		    vd->vd_mmio_base);
	}
#endif /* NGFTTY > 0 */

	/*
	 * Pass 2 at parsing bootinfo now that the MMU is enabled.
	 */
	bootinfo_startup2(nextpa);

	/*
	 * Just use the default pager_map_size for now.  We may decide
	 * to make it larger for large memory configs.
	 */

	/*
	 * We've arranged the kernel to be linked at >= 0x2000,
	 * so we can steal the first 8KB of RAM for the kernel
	 * message buffer.
	 */
	KASSERT(MSGBUFSIZE <= 8192);
	msgbufpa = 0;

	machine_init_common(nextpa);

	/* Check for RND seed from the loader. */
	bootinfo_setup_rndseed();

	char flags[32];
	if (bootinfo_getarg("flags", flags, sizeof(flags))) {
		for (const char *cp = flags; *cp != '\0'; cp++) {
			/* Consume 'm' in favor of BI_RAMDISK. */
			if (*cp == 'm') {
				continue;
			}
			BOOT_FLAG(*cp, boothowto);
		}
	}
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{

	/*
	 * The Goldfish TTY console has already been attached when
	 * the bootinfo was parsed.
	 */

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	struct bootinfo_data *bid = bootinfo_data();

	cpu_startup_common();

	if (bid->bootinfo_mem_segments_ignored) {
		printf("WARNING: ignored %zd bytes of memory in %d segments.\n",
		    bid->bootinfo_mem_segments_ignored_bytes,
		    bid->bootinfo_mem_segments_ignored);
	}
}

void
machine_set_model(void)
{
	struct bi_record *bi = bootinfo_find(BI_VIRT_QEMU_VERSION);
	if (bi != NULL) {
		uint32_t qvers = bootinfo_get_u32(bi);
		cpu_setmodel("Qemu %d.%d.%d Virt platform",
		    (qvers >> 24) & 0xff,
		    (qvers >> 16) & 0xff,
		    (qvers >> 8)  & 0xff);
	} else {
		/* XXX Assume Nono. */
		cpu_setmodel("Nono Virt platform");
	}
}

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{
	/*
	 * virt68k obviously does not have a non-working /RMC, but we
	 * provide this as a r/w node in order to faciliate testing.
	 */
	static bool broken_rmc;

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
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "broken_rmc", NULL,
		       NULL, 0, &broken_rmc, 0,
		       CTL_MACHDEP, CPU_BROKEN_RMC, CTL_EOL);
}

static void
default_reset_func(void *arg, int howto)
{
	printf("WARNING: No reset handler, holding here.\n\n");
	for (;;) {
		/* spin forever. */
	}
}

static void (*cpu_reset_func)(void *, int) = default_reset_func;
static void *cpu_reset_func_arg;

void
cpu_set_reset_func(void (*func)(void *, int), void *arg)
{
	if (cpu_reset_func == default_reset_func && func != NULL) {
		cpu_reset_func = func;
		cpu_reset_func_arg = arg;
	}
}

void
machine_halt(void)
{
	(*cpu_reset_func)(cpu_reset_func_arg, RB_HALT);
}

void
machine_reboot(int howto, char *bootstr)
{
	(*cpu_reset_func)(cpu_reset_func_arg, RB_AUTOBOOT);
}

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
int
nmihand(void *arg)
{

	printf("NMI ignored.\n");

	return 1;
}

const uint16_t ipl2psl_table[NIPL] = {
	[IPL_NONE]       = PSL_S | PSL_IPL0,
	[IPL_SOFTCLOCK]  = PSL_S | PSL_IPL1,
	[IPL_SOFTBIO]    = PSL_S | PSL_IPL1,
	[IPL_SOFTNET]    = PSL_S | PSL_IPL1,
	[IPL_SOFTSERIAL] = PSL_S | PSL_IPL1,
	[IPL_VM]         = PSL_S | PSL_IPL5,
	[IPL_SCHED]      = PSL_S | PSL_IPL6,
	[IPL_HIGH]       = PSL_S | PSL_IPL7,
};
