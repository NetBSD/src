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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/buf.h>
#include <sys/map.h>
#include <sys/exec.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>

#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <vm/vm_kern.h>

#include <machine/signal.h>
#include <machine/frame.h>
#include <machine/bootconfig.h>
#include <machine/cpu.h>
#include <machine/irqhandler.h>
#include <machine/pte.h>
#include <machine/undefined.h>
#include <machine/rtc.h>

#include "ipkdb.h"
#include "md.h"

#include <dev/ofw/openfirm.h>
#include <machine/ofw.h>


/*
 *  Imported types
 */
typedef	struct {
        vm_offset_t physical;
        vm_offset_t virtual;
} pv_addr_t;


/*
 *  Imported variables
 */
extern pv_addr_t undstack;
extern pv_addr_t abtstack;
extern pv_addr_t kernelstack;
extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;
extern int pmap_debug_level;
extern int bufpages;


/*
 *  Imported routines
 */
extern void data_abort_handler		__P((trapframe_t *frame));
extern void prefetch_abort_handler	__P((trapframe_t *frame));
extern void undefinedinstruction_bounce	__P((trapframe_t *frame));
extern void pmap_debug	__P((int level));
#ifdef	DDB
extern void db_machine_init     __P((void));
#endif
extern char *strstr		__P((char *s1, char *s2));
extern u_long strtoul		__P((const char *s, char **ptr, int base));

/*
 *  Exported variables
 */
BootConfig bootconfig;
char *boot_path;
char *boot_args;
int debug_flags;
int max_processes;
int cpu_cache;
#if NMD > 0
u_int memory_disc_size;
#endif

int ofw_handleticks = 0;	/* set to TRUE by cpu_initclocks */


/**************************************************************/


/*
 * void boot(int howto, char *bootstr)
 *
 * Reboots the system, in event of:
 *   - reboot system call
 *   - panic
 */

void
boot(howto, bootstr)
	int howto;
	char *bootstr;
{
    /* Just call OFW common routine. */
    ofw_boot(howto, bootstr);

    OF_boot("not reached -- stupid compiler");
}


/*
 * vm_offset_t initarm(ofw_handle_t handle)
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
vm_offset_t
initarm(ofw_handle)
    ofw_handle_t ofw_handle;
{
    set_cpufuncs();

    /* XXX - set these somewhere else? -JJK */
    max_processes = 64;
    debug_flags = 0;
    boothowto = 0;
    cpu_cache =	0x3;	    /* IC_ENABLE | DC_ENABLE | WBUF_ENABLE */

    /* Init the OFW interface. */
    /* MUST do this before invoking any OFW client services! */
    ofw_init(ofw_handle);

    /* Initialize the console (which will call into OFW). */
    /* This will allow us to see panic messages and other printf output. */
    consinit();

    /* Get boot info and process it. */
    ofw_getbootinfo(&boot_path, &boot_args);
    process_kernel_args();

    /* Configure memory. */
    ofw_configmem();

    /* Set-up stacks. */
    {
	/* OFW has control of the interrupt frame. */
	/* The kernel stack for SVC mode will be updated on return from this routine. */
	/* All we need to do is prepare for abort-handling and undefined exceptions. */
	set_stackptr(PSR_UND32_MODE, undstack.virtual + NBPG);
	set_stackptr(PSR_ABT32_MODE, abtstack.virtual + NBPG);
    }

    /* Set-up exception handlers. */
    {
	/* Take control of selected vectors from OFW. */
	/* We take:  undefined, swi, pre-fetch abort, data abort, addrexc */
	/* OFW retains:  reset, irq, fiq */
	{
	    int our_vecnums[] = {1, 2, 3, 4, 5};
	    unsigned int *vectors = (unsigned int *)0;
	    extern unsigned int page0[];
	    int i;

	    for (i = 0; i < (sizeof(our_vecnums) / sizeof(int)); i++) {
		int vecnum = our_vecnums[i];

		/* Copy both the instruction and the data word for the vector. */
		/* The latter is needed to support branching arbitrarily far. */
		vectors[vecnum] = page0[vecnum];
		vectors[vecnum+8] = page0[vecnum+8];
	    }

	    cpu_cache_syncI();
	}

	data_abort_handler_address = (u_int)data_abort_handler;
	prefetch_abort_handler_address = (u_int)prefetch_abort_handler;
	undefined_handler_address =
	  (u_int)undefinedinstruction_bounce;	/* why is this needed? -JJK */
	cpu_cache_syncI();			/* XXX - Is this really needed */

	/* Initialise the undefined instruction handlers. */
	undefined_init();
	cpu_cache_syncI();			/* XXX - Is this really needed */

	/* Set-up the IRQ system. */
	irq_init();
    }

#ifdef DDB
    printf("ddb: ");
    db_machine_init();
    ddb_init();

    if (boothowto & RB_KDB)
	Debugger();
#endif

    /* Return the new stackbase. */
    return(kernelstack.virtual + USPACE_SVC_STACK_TOP);
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
void
process_kernel_args(void)
{
    /* Check for MI args. */
    {
	char *p, *q;

	q = boot_args;
	while ((p = strstr(q, "-")) != NULL) {
	    p++;
	    switch (*p) {
		case 's':
		    boothowto |= RB_SINGLE;
		    break;
		case 'a':
		    boothowto |= RB_ASKNAME;
		    break;
		case 'k':
		    boothowto |= RB_KDB;
		    break;
		default:
		    break;
	    }
	    q = p;
	}
    }

    /* Check for arm-specific args. */
    {
	char *args, *ptr;

	if (strstr(args, "nocache"))
	    cpu_cache &= ~1;

	if (strstr(args, "nowritebuf"))
	    cpu_cache &= ~2;

	if (strstr(args, "fpaclk2"))
	    cpu_cache |= 4;

	if (strstr(args, "icache"))
	    cpu_cache |= 8;

	if (strstr(args, "dcache"))
	    cpu_cache |= 16;

	ptr = strstr(args, "maxproc=");
	if (ptr) {
	    max_processes = (int)strtoul(ptr + 8, NULL, 10);
	    if (max_processes < 16)
		max_processes = 16;
	    /* Limit is PDSIZE * (maxprocess+1) <= 4MB */
	    if (max_processes > 255)
		max_processes = 255;
	}

#if NMD > 0
	ptr = strstr(args, "memorydisc=");
	if (!ptr)
	    ptr = strstr(args, "memorydisk=");
	if (ptr) {
	    memory_disc_size = (u_int)strtoul(ptr + 11, NULL, 10);
	    memory_disc_size *= 1024;
	    if (memory_disc_size < 32*1024)
		memory_disc_size = 32*1024;
	    if (memory_disc_size > 2048*1024)
		memory_disc_size = 2048*1024;
	}
#endif

	if (strstr(args, "single"))
	    boothowto |= RB_SINGLE;

	if (strstr(args, "kdb"))
	    boothowto |= RB_KDB;

	ptr = strstr(args, "pmapdebug=");
	if (ptr) {
	    pmap_debug_level = (int)strtoul(ptr + 10, NULL, 10);
	    pmap_debug(pmap_debug_level);
	    debug_flags |= 0x01;
	}

	ptr = strstr(args, "nbuf=");
	if (ptr)
	    bufpages = (int)strtoul(ptr + 5, NULL, 10);
    }

    /* Check for ofwgencfg-specific args. */
    {
	/* None at the moment. */
    }
}


/*
 *  Walk the OFW device tree and configure found devices.
 *
 *  Move this into common OFW module? -JJK
 */
void
ofrootfound()
{
    int node;
    struct ofprobe probe;

    if (!(node = OF_peer(0)))
	panic("No OFW root");
    probe.phandle = node;
    if (!config_rootfound("ofroot", &probe))
	panic("ofroot not configured");
}
