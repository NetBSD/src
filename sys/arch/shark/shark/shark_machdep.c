/*	$NetBSD: shark_machdep.c,v 1.40.2.1 2014/08/20 00:03:24 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: shark_machdep.c,v 1.40.2.1 2014/08/20 00:03:24 tls Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/ksyms.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>
#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <arm/fiq.h>
#include <arm/locore.h>
#include <arm/undefined.h>
#include <arm/arm32/machdep.h>

#include <machine/bootconfig.h>
#include <machine/pio.h>
#include <machine/ofw.h>
#include <machine/isa_machdep.h>
#include <shark/shark/sequoia.h>

#include "isadma.h"

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

#include "ksyms.h"

/*
 *  Imported variables
 */
extern pv_addr_t irqstack;
extern pv_addr_t undstack;
extern pv_addr_t abtstack;

/*
 *  Imported routines
 */
extern void data_abort_handler(trapframe_t *frame);
extern void prefetch_abort_handler(trapframe_t *frame);
extern void undefinedinstruction_bounce(trapframe_t *frame);
extern void consinit(void);
int	ofbus_match(device_t, cfdata_t, void *);
void	ofbus_attach(device_t, device_t, void *);


paddr_t isa_io_physaddr, isa_mem_physaddr;

/*
 *  Exported variables
 */
BootConfig bootconfig;
char *boot_args = NULL;
char *boot_file = NULL;
extern char *booted_kernel;
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
extern unsigned int sa1_cache_clean_addr;
extern unsigned int sa1_cache_clean_size;

CFATTACH_DECL_NEW(ofbus_root, 0,
    ofbus_match, ofbus_attach, NULL, NULL);

/*
 *  Exported routines
 */
/* Move to header file? */
extern void cpu_reboot(int, char *);
extern void ofrootfound(void);

/* Local routines */
static void process_kernel_args(void);
void ofw_device_register(device_t, void *);

/* Kernel text starts at the base of the kernel address space. */
#define	KERNEL_TEXT_BASE	(KERNEL_BASE + 0x00000000)

/**************************************************************/


/*
 * void cpu_reboot(int howto, char *bootstr)
 *
 * Reboots the system, in evsent of:
 *   - reboot system call
 *   - panic
 */

void
cpu_reboot(int howto, char *bootstr)
{
	/* Just call OFW common routine. */
	ofw_boot(howto, bootstr);

	OF_boot("not reached -- stupid compiler");
}

/*
 * u_int initarm(void *handle)
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

struct fiqhandler shark_fiqhandler;
struct fiqregs shark_fiqregs;

u_int
initarm(void *arg)
{
	ofw_handle_t ofw_handle = arg;
	paddr_t  pclean;
	vaddr_t  isa_io_virtaddr, isa_mem_virtaddr;
	paddr_t  isadmaphysbufs;
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
	isa_mem_virtaddr = ofw_map(isa_mem_physaddr, L1_S_SIZE, 0);
	isa_io_virtaddr  = ofw_map(isa_io_physaddr,  L1_S_SIZE, 0);

	/* Set-up the ISA system: must be done before consinit */
	isa_init(isa_io_virtaddr, isa_mem_virtaddr);
  
	/* Initialize the console (which will call into OFW). */
	/* This will allow us to see panic messages and other printf output. */
	consinit();

	/* Get boot info and process it. */
	ofw_getbootinfo(&boot_file, &boot_args);
	process_kernel_args();

	ofw_configisadma(&isadmaphysbufs);
#if (NISADMA > 0)
	isa_dma_init();
#endif

	/* allocate a cache clean space */
	if ((pclean = ofw_getcleaninfo()) != -1) {
		sa1_cache_clean_addr = ofw_map(pclean, 0x4000 * 2,
		     L2_B | L2_C);
		sa1_cache_clean_size = 0x4000;
	}

	/* Configure memory. */
	ofw_configmem();

	/*
	 * Set-up stacks.
	 * The kernel stack for SVC mode will be updated on return
	 * from this routine.
	 */
	set_stackptr(PSR_IRQ32_MODE, irqstack.pv_va + PAGE_SIZE);
	set_stackptr(PSR_UND32_MODE, undstack.pv_va + PAGE_SIZE);
	set_stackptr(PSR_ABT32_MODE, abtstack.pv_va + PAGE_SIZE);

	/* Set-up exception handlers. */

	/*
	 * Take control of selected vectors from OFW.
	 * We take: undefined, swi, pre-fetch abort, data abort, addrexc,
         * 	    irq, fiq
	 * OFW retains:  reset
         */
	arm32_vector_init(ARM_VECTORS_LOW, ARM_VEC_ALL & ~ARM_VEC_RESET);

	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address =
	    (u_int)undefinedinstruction_bounce;	/* why is this needed? -JJK */

	/* Initialise the undefined instruction handlers. */
	undefined_init();

	/* Now for the SHARK-specific part of the FIQ set-up */
	shark_fiqhandler.fh_func = shark_fiq;
	shark_fiqhandler.fh_size = shark_fiq_end - shark_fiq;
	shark_fiqhandler.fh_flags = 0;
	shark_fiqhandler.fh_regs = &shark_fiqregs;

	shark_fiqregs.fr_r8   = isa_io_virtaddr;
	shark_fiqregs.fr_r9   = 0; /* no routine right now */
	shark_fiqregs.fr_r10  = 0; /* no arg right now */
	shark_fiqregs.fr_r11  = 0; /* scratch */
	shark_fiqregs.fr_r12  = 0; /* scratch */
	shark_fiqregs.fr_r13  = 0; /* must set a stack when r9 is set! */

	if (fiq_claim(&shark_fiqhandler))
		panic("Cannot claim FIQ vector.");

#if NKSYMS || defined(DDB) || defined(MODULAR)
#ifndef __ELF__
	{
		struct exec *kernexec = (struct exec *)KERNEL_TEXT_BASE;
		extern int end;
		extern char *esym;

		ksyms_addsyms_elf(kernexec->a_syms, &end, esym);
	}
#endif /* __ELF__ */
#endif /* NKSYMS || defined(DDB) || defined(MODULAR) */

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
 */
static void
process_kernel_args(void)
{
#ifdef RB_QUIET
	int value;
#endif
	
#if defined(RB_QUIET) && defined(BOOT_QUIETLY)
	boothowto |= RB_QUIET;
#endif

	/* Process all the generic ARM boot options */
	parse_mi_bootargs(boot_args);

#ifdef RB_QUIET
	if (get_bootconf_option(args, "noquiet", BOOTOPT_TYPE_BOOLEAN, &value)) {
		if (!value)
			boothowto |= RB_QUIET;
		else
			boothowto &= ~RB_QUIET;
	}
	if (get_bootconf_option(args, "quiet", BOOTOPT_TYPE_BOOLEAN, &value)) {
		if (value)
			boothowto |= RB_QUIET;
		else
			boothowto &= ~RB_QUIET;
	}
#endif

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

void
ofw_device_register(device_t dev, void *aux)
{
	static device_t parent;
#if NSD > 0 || NCD > 0
	static device_t scsipidev;
#endif
	static char *boot_component;
	struct ofbus_attach_args *oba;
	char name[64];
	int i;

	if (boot_component == NULL) {
		char *cp;
		boot_component = boot_file;
		if (boot_component == NULL)
			return;
		cp = strrchr(boot_component, ':');
		if (cp != NULL) {
			int cplen;
			*cp++ = '\0';
			if (cp[0] == '\\')
				cp++;
			booted_kernel = cp; 

			/* Zap ".aout" suffix, arm32 libkvm now requires ELF */
			cplen = strlen(cp);
			if (cplen > 5 && !strcmp(&cp[cplen-5], ".aout")) {
				cp[cplen-5] = '\0';
			}
		}
	}

	if (booted_device != NULL
	    || boot_component == NULL
	    || boot_component[0] == '\0')
		return;

	if (device_is_a(dev, "ofbus") || device_is_a(dev, "ofisa")) {
		oba = aux;
	} else if (parent == NULL) {
		return;
	} else if (parent == device_parent(dev)
		   && device_is_a(parent, "ofisa")) {
		struct ofisa_attach_args *aa = aux;
		oba = &aa->oba;
#if NWD > 0 || NSD > 0 || NCD > 0
	} else if (device_parent(device_parent(dev)) != NULL
		   && parent == device_parent(device_parent(dev))
		   && device_is_a(parent, "wdc")) {
#if NSD > 0 || NCD > 0
		if (device_is_a(dev, "atapibus")) {
			scsipidev = dev;
			return;
		}
#endif
#if NWD > 0
		if (device_is_a(dev, "wd")) {
			struct ata_device *adev = aux;
			char *cp = strchr(boot_component, '@');
			if (cp != NULL
			    && adev->adev_drv_data->drive == strtoul(cp+1, NULL, 16)
			    && adev->adev_channel == 0) {
				booted_device = dev;
				return;
			}
		}
		return;
#endif /* NWD > 0 */
#if NSD > 0 || NCD > 0
	} else if (scsipidev == device_parent(dev)
	    && (device_is_a(dev, "sd") || device_is_a(dev, "cd"))) {
		struct scsipibus_attach_args *sa = aux;
		char *cp = strchr(boot_component, '@');
		if (cp != NULL
		    && sa->sa_periph->periph_channel->chan_bustype->bustype_type == SCSIPI_BUSTYPE_ATAPI
		    && sa->sa_periph->periph_channel->chan_channel == 0
		    && sa->sa_periph->periph_target == strtoul(cp+1, NULL, 16)) {
			booted_device = dev;
		}
		return;
#endif /* NSD > 0 || NCD > 0 */
#endif /* NWD > 0 || NSD > 0 || NCD > 0 */
	} else {
		return;
	}
	(void) of_packagename(oba->oba_phandle, name, sizeof name);
	i = strlen(name);
	if (!strncmp(name, &boot_component[1], i)
	    && (boot_component[i+1] == '/' || boot_component[i+1] == '\0')) {
		boot_component += i + 1;
		if (boot_component[0] == '/') {
			parent = dev;
		} else {
			booted_device = dev;
		}
	}
}
