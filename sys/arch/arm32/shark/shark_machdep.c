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
#include <vm/vm.h>
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
#include <machine/pio.h>
#include <machine/pte.h>
#include <machine/undefined.h>
#include <machine/rtc.h>

#include "ipkdb.h"
#include "md.h"

#include <dev/ofw/openfirm.h>
#include <machine/ofw.h>
#include <arm32/isa/isa_machdep.h>
#include <arm32/shark/sequoia.h>

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
extern pv_addr_t irqstack;
extern pv_addr_t undstack;
extern pv_addr_t abtstack;
extern pv_addr_t kernelstack;
extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
extern u_int undefined_handler_address;
#ifdef PMAP_DEBUG
extern int pmap_debug_level;
#endif
extern int bufpages;

/*
 *  Imported routines
 */
extern void data_abort_handler		__P((trapframe_t *frame));
extern void prefetch_abort_handler	__P((trapframe_t *frame));
extern void undefinedinstruction_bounce	__P((trapframe_t *frame));
#ifdef PMAP_DEBUG
extern void pmap_debug	__P((int level));
#endif
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

vm_offset_t cache_clean_vms[2] = {0, 0}; /* vms = plural of vm, not VMS! :-) */

int ofw_handleticks = 0;	/* set to TRUE by cpu_initclocks */


/*
 *  Exported routines
 */
/* Move to header file? */
extern void boot		__P((int, char *));
extern vm_offset_t initarm	__P((ofw_handle_t));
extern void consinit		__P((void));
extern void process_kernel_args	__P((void));
extern void ofrootfound		__P((void));


/**************************************************************/


/*
 * void boot(int howto, char *bootstr)
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
  vm_offset_t  pclean, clean0, clean1;
  vm_offset_t  isa_io_physaddr, isa_mem_physaddr;
  vm_offset_t  isa_io_virtaddr, isa_mem_virtaddr;
  vm_offset_t  isadmaphysbufs;
  fiqhandler_t fiqhandler;
  extern void  shark_fiq     __P((void));
  extern void  shark_fiq_end __P((void));

    /* Don't want to get hit with interrupts 'til we're ready. */
    (void)disable_interrupts(I32_bit | F32_bit);

    set_cpufuncs();

    /* XXX - set these somewhere else? -JJK */
    max_processes = 64;
    debug_flags = 0;
    boothowto = 0;
    cpu_cache =	0x3;	    /* IC_ENABLE | DC_ENABLE | WBUF_ENABLE */

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
    ofw_getbootinfo(&boot_path, &boot_args);
    process_kernel_args();

    ofw_configisadma(&isadmaphysbufs);
    isaphysmem = ofw_map(isadmaphysbufs, 
			 (DMA_BOUNCE * NBPG + DMA_LARGE_BUFFER_SIZE), 0);

    /* allocate a cache clean space */
    if ((pclean = ofw_getcleaninfo()) != -1) {
      clean0 = ofw_map(pclean, NBPD, PT_B | PT_C);
      /* ...and another one: see the comment in the cache flush code =
	 arm32/arm32/cpufunc_asm.S */
      clean1 = ofw_map(pclean, NBPD, PT_B | PT_C);
      /* don't set cache_clean_vm until after it is mapped */
      cache_clean_vms[0] = clean0;
      cache_clean_vms[1] = clean1;
    }

    /* Configure memory. */
    ofw_configmem();

    /* Set-up stacks. */
    {
	/* The kernel stack for SVC mode will be updated on return from this routine. */
	set_stackptr(PSR_IRQ32_MODE, irqstack.virtual + NBPG);
	set_stackptr(PSR_UND32_MODE, undstack.virtual + NBPG);
	set_stackptr(PSR_ABT32_MODE, abtstack.virtual + NBPG);
    }

    /* Set-up exception handlers. */
    {
	/* Take control of selected vectors from OFW. */
	/* We take:  undefined, swi, pre-fetch abort, data abort, addrexc,
                     irq, fiq
	   OFW retains:  reset
        */
	{
	    int our_vecnums[] = {1, 2, 3, 4, 5, 6, 7};
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

	/* Now for the SHARK-specific part of the FIQ set-up */
	fiqhandler.fh_func = shark_fiq;
	fiqhandler.fh_size = shark_fiq_end - shark_fiq;
	fiqhandler.fh_mask = 0x01; /* XXX ??? */
	fiqhandler.fh_r8   = isa_io_virtaddr;
	fiqhandler.fh_r9   = 0; /* no routine right now */
	fiqhandler.fh_r10  = 0; /* no arg right now */
	fiqhandler.fh_r11  = 0; /* scratch */
	fiqhandler.fh_r12  = 0; /* scratch */
	fiqhandler.fh_r13  = 0; /* must set a stack when r9 is set! */

	if (fiq_claim(&fiqhandler)) panic("Cannot claim FIQ vector.\n");

    }

#ifdef DDB
    printf("ddb: ");
    db_machine_init();
  {
    struct exec *kernexec = (struct exec *)KERNEL_BASE;
    extern int end;
    extern char *esym;

    ddb_init(kernexec->a_syms, &end, esym);
  }
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

#ifdef BOOT_QUIETLY
    boothowto |= RB_QUIET;
#endif

    /* Check for MI args. */
    {
	char *p, *q;

	for (q = boot_args;  *q && (p = strstr(q, "-")) != NULL; q = p) {
	    /*
	     * Be sure the arguments are surrounded by space or NUL
	     */
	    if(p > boot_args && p[-1] != ' ') {
		p++;
		continue;
	    }
	    if(*++p == '\0')
		break;
	    if(p[1] == ' ' || p[1] == '\0') {
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
	    }
	}
    }

    /* Check for arm-specific args. */
    {
	char *args = boot_args, *ptr;

	if (ptr = strstr(args, "nocache")) {
	    if((ptr > boot_args || ptr[-1] == ' ') &&
	       (ptr[7] == ' ' || ptr[7] == '\0'))
		    cpu_cache &= ~1;
	}

	if (ptr = strstr(args, "nowritebuf")) {
	    if((ptr > boot_args || ptr[-1] == ' ') &&
	       (ptr[10] == ' ' || ptr[10] == '\0'))
		    cpu_cache &= ~2;
	}

	if (ptr = strstr(args, "fpaclk2")) {
	    if((ptr > boot_args || ptr[-1] == ' ') &&
	       (ptr[7] == ' ' || ptr[7] == '\0'))
		    cpu_cache |= 4;
	}

	if (ptr = strstr(args, "icache")) {
	    if((ptr > boot_args || ptr[-1] == ' ') &&
	       (ptr[6] == ' ' || ptr[6] == '\0'))
		    cpu_cache |= 8;
	}

	if (ptr = strstr(args, "dcache")) {
	    if((ptr > boot_args || ptr[-1] == ' ') &&
	       (ptr[6] == ' ' || ptr[6] == '\0'))
		    cpu_cache |= 16;
	}

	ptr = strstr(args, "maxproc=");
	if (ptr && (ptr > boot_args || ptr[-1] == ' ')) {
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
	if (ptr && (ptr > boot_args || ptr[-1] == ' ')) {
	    memory_disc_size = (u_int)strtoul(ptr + 11, NULL, 10);
	    memory_disc_size *= 1024;
	    if (memory_disc_size < 32*1024)
		memory_disc_size = 32*1024;
	    if (memory_disc_size > 2048*1024)
		memory_disc_size = 2048*1024;
	}
#endif

	if (ptr = strstr(args, "single")) {
	    if((ptr > boot_args || ptr[-1] == ' ') &&
	       (ptr[6] == ' ' || ptr[6] == '\0'))
		    boothowto |= RB_SINGLE;
	}

	if (ptr = strstr(args, "kdb")) {
	    if((ptr > boot_args || ptr[-1] == ' ') &&
	       (ptr[3] == ' ' || ptr[3] == '\0'))
		    boothowto |= RB_KDB;
	}

#ifdef PMAP_DEBUG
	ptr = strstr(args, "pmapdebug=");
	if (ptr && (ptr > boot_args || ptr[-1] == ' ')) {
	    pmap_debug_level = (int)strtoul(ptr + 10, NULL, 10);
	    pmap_debug(pmap_debug_level);
	    debug_flags |= 0x01;
	}
#endif

	ptr = strstr(args, "nbuf=");
	if (ptr && (ptr > boot_args || ptr[-1] == ' '))
	    bufpages = (int)strtoul(ptr + 5, NULL, 10);

	if (strstr(args, "noquiet"))
	    boothowto &= ~RB_QUIET;
	else if (strstr(args, "quiet"))
	    boothowto |= RB_QUIET;
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
ofrootfound(void)
{
    int node;
    struct ofprobe probe;

    if (!(node = OF_peer(0)))
	panic("No OFW root");
    probe.phandle = node;
    if (!config_rootfound("ofroot", &probe))
	panic("ofroot not configured");
}
