/*	$NetBSD: ofwgencfg_machdep.c,v 1.11.34.1 2008/01/20 17:51:05 bouyer Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 *  Kernel setup for the OFW Generic Configuration
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofwgencfg_machdep.c,v 1.11.34.1 2008/01/20 17:51:05 bouyer Exp $");

#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/frame.h>
#include <machine/bootconfig.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/irqhandler.h>
#include <arm/arm32/machdep.h>
#include <arm/undefined.h>

#include <dev/ofw/openfirm.h>
#include <machine/ofw.h>

#include "ksyms.h"

/*
 *  Imported variables
 */
extern pv_addr_t undstack;
extern pv_addr_t abtstack;
extern pv_addr_t kernelstack;
extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

/*
 *  Imported routines
 */
extern void data_abort_handler		__P((trapframe_t *frame));
extern void prefetch_abort_handler	__P((trapframe_t *frame));
extern void undefinedinstruction_bounce	__P((trapframe_t *frame));
int	ofbus_match __P((struct device *, struct cfdata *, void *));
void	ofbus_attach __P((struct device *, struct device *, void *));

/* Local routines */
static void process_kernel_args	__P((void));

/*
 *  Exported variables
 */
BootConfig bootconfig;
char *boot_args = NULL;
char *boot_file = NULL;
#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif	/* !PMAP_STATIC_L1S */

int ofw_handleticks = 0;	/* set to TRUE by cpu_initclocks */

CFATTACH_DECL(ofbus_root, sizeof(struct device),
    ofbus_match, ofbus_attach, NULL, NULL);

/**************************************************************/


/*
 * void boot(int howto, char *bootstr)
 *
 * Reboots the system, in event of:
 *   - reboot system call
 *   - panic
 */

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	/* Just call OFW common routine. */
	ofw_boot(howto, bootstr);

	OF_boot("not reached -- stupid compiler");
}


/*
 * u_int initarm(ofw_handle_t handle)
 *
 * Initial entry point on startup for a GENERIC OFW
 * system.  Called with MMU on, running in the OFW
 * client environment.
 *
 * Major tasks are:
 *  - read the bootargs out of OFW;
 *  - take over memory management from OFW;
 *  - set-up the stacks
 *  - set-up the exception handlers
 * 
 * Return the new stackptr (va) for the SVC frame.
 *
 */
u_int
initarm(void *cookie)
{
	ofw_handle_t ofw_handle = cookie;

	set_cpufuncs();

	/* XXX - set this somewhere else? -JJK */
	boothowto = 0;

	/* Init the OFW interface. */
	/* MUST do this before invoking any OFW client services! */
	ofw_init(ofw_handle);

	/* Initialize the console (which will call into OFW). */
	/* This will allow us to see panic messages and other printf output. */
	consinit();

	/* Get boot info and process it. */
	ofw_getbootinfo(&boot_file, &boot_args);
	process_kernel_args();

	/* Configure memory. */
	ofw_configmem();

	/*
	 * Set-up stacks.
	 * OFW has control of the interrupt frame.
	 * The kernel stack for SVC mode will be updated on return from
	 * this routine. All we need to do is prepare for abort-handling 
	 * and undefined exceptions.
	 */
	set_stackptr(PSR_UND32_MODE, undstack.pv_va + PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + PAGE_SIZE);

	/* Set-up exception handlers.
	 * Take control of selected vectors from OFW.
	 * We take:  undefined, swi, pre-fetch abort, data abort, addrexc
	 * OFW retains:  reset, irq, fiq
	 */
	arm32_vector_init(ARM_VECTORS_LOW,
	    ARM_VEC_ALL & ~(ARM_VEC_RESET|ARM_VEC_IRQ|ARM_VEC_FIQ));

	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address =
	    (u_int)undefinedinstruction_bounce;	/* why is this needed? -JJK */

	/* Initialise the undefined instruction handlers. */
	undefined_init();

	/* Set-up the IRQ system. */
	irq_init();

#if NKSYMS || defined(DDB) || defined(LKM)
#ifdef __ELF__
	ksyms_init(0, NULL, NULL);	/* XXX */
#else
	{
		struct exec *kernexec = (struct exec *)KERNEL_TEXT_BASE;
		extern int end;
		extern char *esym;

		ksyms_init(kernexec->a_syms, &end, esym);
	}
#endif /* __ELF__ */
#endif

#ifdef DDB
	db_machine_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* Return the new stackbase. */
	return(kernelstack.pv_va + USPACE_SVC_STACK_TOP);
}


/*
 *  Set various globals based on contents of boot_args
 *
 *  Note that this routine must NOT trash boot_args, as
 *  it is scanned by later routines.
 *
 *  There ought to be a routine in machdep.c that does
 *  the generic bits of this. -JJK
 */
static void
process_kernel_args(void)
{
	/* Process all the generic ARM boot options */
	parse_mi_bootargs(boot_args);

	/* Check for ofwgencfg-specific args here. */
}


/*
 *  Walk the OFW device tree and configure found devices.
 *
 *  Move this into common OFW module? -JJK
 */
void
ofrootfound(void)
{
	int node;
	struct ofbus_attach_args aa;

	if (!(node = OF_peer(0)))
		panic("No OFW root");
	aa.oba_busname = "ofw";
	aa.oba_phandle = node;
	if (!config_rootfound("ofbus", &aa))
		panic("ofw root ofbus not configured");
}
