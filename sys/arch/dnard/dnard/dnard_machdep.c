/*	$NetBSD: dnard_machdep.c,v 1.1.2.2 2002/02/11 20:07:37 jdolecek Exp $	*/

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
 *  Kernel setup for the SHARK Configuration
 */

#include "opt_ddb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <arm/fiq.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/frame.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pio.h>
#include <arm/undefined.h>

#include "opt_ipkdb.h"

#include <dev/ofw/openfirm.h>
#include <machine/ofw.h>
#include <machine/isa_machdep.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>
#include <dnard/dnard/sequoia.h>

#include "wd.h"
#include "cd.h"
#include "sd.h"

#if NWD > 0 || NSD > 0 || NCD > 0
#include <dev/ata/atavar.h>
#endif
#if NSD > 0 || NCD > 0
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>
#endif

/*
 *  Imported variables
 */
extern pv_addr_t irqstack;
extern pv_addr_t undstack;
extern pv_addr_t abtstack;
extern pv_addr_t kernelstack;
extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;

/*
 *  Imported routines
 */
extern void parse_mi_bootargs		__P((char *args));
extern void data_abort_handler		__P((trapframe_t *frame));
extern void prefetch_abort_handler	__P((trapframe_t *frame));
extern void undefinedinstruction_bounce	__P((trapframe_t *frame));
extern void consinit		__P((void));
int	ofbus_match __P((struct device *, struct cfdata *, void *));
void	ofbus_attach __P((struct device *, struct device *, void *));


/*
 *  Exported variables
 */
char *boot_args = NULL;
char *boot_file = NULL;
char *boot_kernel = NULL;
#ifndef PMAP_STATIC_L1S
int max_processes = 64;			/* Default number */
#endif	/* !PMAP_STATIC_L1S */

int ofw_handleticks = 0;	/* set to TRUE by cpu_initclocks */

/*
 * For faster cache cleaning we need two 16K banks of virtual address
 * space that NOTHING else will access and then we alternate the cache
 * cleaning between the two banks.
 * The cache cleaning code requires requires 2 banks aligned
 * on total size boundry so the banks can be alternated by
 * xorring the size bit (assumes the bank size is a power of 2)
 */
extern unsigned int sa110_cache_clean_addr;
extern unsigned int sa110_cache_clean_size;

struct cfattach ofbus_root_ca = {
	sizeof(struct device), ofbus_match, ofbus_attach
};


/*
 *  Exported routines
 */
/* Move to header file? */
extern void cpu_reboot		__P((int, char *));
extern vaddr_t initarm		__P((ofw_handle_t));

/* Local routines */
static void process_kernel_args	__P((void));


/**************************************************************/


/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system, in evsent of:
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
 * vaddr_t initarm(ofw_handle_t handle)
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

struct fiqhandler dnard_fiqhandler;
struct fiqregs dnard_fiqregs;

vaddr_t
initarm(ofw_handle)
	ofw_handle_t ofw_handle;
{
	vaddr_t  pclean;
	vaddr_t  isa_io_physaddr, isa_mem_physaddr;
	vaddr_t  isa_io_virtaddr, isa_mem_virtaddr;
	vaddr_t  isadmaphysbufs;
	extern char shark_fiq[], shark_fiq_end[];

	/* Don't want to get hit with interrupts 'til we're ready. */
	(void)disable_interrupts(I32_bit | F32_bit);

	set_cpufuncs();

	/* XXX - set these somewhere else? -JJK */
	boothowto = 0;

	/* Init the OFW interface. */
	/* MUST do this before invoking any OFW client services! */
	ofw_init(ofw_handle);

	/* Configure ISA stuff: must be done before consinit */
	ofw_configisa(&isa_io_physaddr, &isa_mem_physaddr);

	/* Map-in ISA I/O and memory space. */
	/* XXX - this should be done in the isa-bus attach routine! -JJK */
	isa_mem_virtaddr = ofw_map(isa_mem_physaddr, NBPD, 0);
	isa_io_virtaddr  = ofw_map(isa_io_physaddr,  NBPD, 0);

	/* Set-up the ISA system: must be done before consinit */
	isa_init(isa_io_virtaddr, isa_mem_virtaddr);
  
	/* Initialize the console (which will call into OFW). */
	/* This will allow us to see panic messages and other printf output. */
	consinit();

	/* Get boot info and process it. */
	ofw_getbootinfo(&boot_file, &boot_args);
	process_kernel_args();

	ofw_configisadma(&isadmaphysbufs);
	isa_dma_init();

	/* allocate a cache clean space */
	if ((pclean = ofw_getcleaninfo()) != -1) {
		sa110_cache_clean_addr = ofw_map(pclean, 0x4000 * 2,
		     PT_B | PT_C);
		sa110_cache_clean_size = 0x4000;
	}

	/* Configure memory. */
	ofw_configmem();

	/*
	 * Set-up stacks.
	 * The kernel stack for SVC mode will be updated on return
	 * from this routine.
	 */
	set_stackptr(PSR_IRQ32_MODE, irqstack.pv_va + NBPG);
	set_stackptr(PSR_UND32_MODE, undstack.pv_va + NBPG);
	set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + NBPG);

	/* Set-up exception handlers. */

	/*
	 * Take control of selected vectors from OFW.
	 * We take: undefined, swi, pre-fetch abort, data abort, addrexc,
         * 	    irq, fiq
	 * OFW retains:  reset
         */
	{
		int our_vecnums[] = {1, 2, 3, 4, 5, 6, 7};
		unsigned int *vectors = (unsigned int *)0;
		extern unsigned int page0[];
		int i;

		for (i = 0; i < (sizeof(our_vecnums) / sizeof(int)); i++) {
			int vecnum = our_vecnums[i];

			/* Copy both the instruction and the data word
			 * for the vector.
			 * The latter is needed to support branching
			 * arbitrarily far.
			 */
			vectors[vecnum] = page0[vecnum];
			vectors[vecnum + 8] = page0[vecnum + 8];
		}

		/* Sync the first 16 words of memory */
		cpu_icache_sync_range(0, 64);
	}

	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address =
	    (u_int)undefinedinstruction_bounce;	/* why is this needed? -JJK */

	/* Initialise the undefined instruction handlers. */
	undefined_init();

	/* Now for the SHARK-specific part of the FIQ set-up */
	dnard_fiqhandler.fh_func = shark_fiq;
	dnard_fiqhandler.fh_size = (char *)shark_fiq_end - (char *)shark_fiq;
	dnard_fiqhandler.fh_flags = 0;
	dnard_fiqhandler.fh_regs = &dnard_fiqregs;

	dnard_fiqregs.fr_r8   = isa_io_virtaddr;
	dnard_fiqregs.fr_r9   = 0; /* no routine right now */
	dnard_fiqregs.fr_r10  = 0; /* no arg right now */
	dnard_fiqregs.fr_r11  = 0; /* scratch */
	dnard_fiqregs.fr_r12  = 0; /* scratch */
	dnard_fiqregs.fr_r13  = 0; /* must set a stack when r9 is set! */

	if (fiq_claim(&dnard_fiqhandler))
		panic("Cannot claim FIQ vector.\n");

#ifdef DDB
	db_machine_init();
#ifdef __ELF__
	ddb_init(0, NULL, NULL);	/* XXX */
#else
	{
		struct exec *kernexec = (struct exec *)KERNEL_TEXT_BASE;
		extern int end;
		extern char *esym;

		ddb_init(kernexec->a_syms, &end, esym);
	}
#endif /* __ELF__ */

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
 */
static void
process_kernel_args(void)
{
#ifdef RB_QUIET
	int bool;
#endif
	
#if defined(RB_QUIET) && defined(BOOT_QUIETLY)
	boothowto |= RB_QUIET;
#endif

	/* Process all the generic ARM boot options */
	parse_mi_bootargs(boot_args);

#ifdef RB_QUIET
	if (get_bootconf_option(args, "noquiet", BOOTOPT_TYPE_BOOLEAN, &bool)) {
		if (!bool)
			boothowto |= RB_QUIET;
		else
			boothowto &= ~RB_QUIET;
	}
	if (get_bootconf_option(args, "quiet", BOOTOPT_TYPE_BOOLEAN, &bool)) {
		if (bool)
			boothowto |= RB_QUIET;
		else
			boothowto &= ~RB_QUIET;
	}
#endif

	/* Check for ofwgencfg-specific args here. */
}

