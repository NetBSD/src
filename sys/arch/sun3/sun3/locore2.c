/*
 * Copyright (c) 1993 Adam Glass
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
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
 * $Header: /cvsroot/src/sys/arch/sun3/sun3/locore2.c,v 1.14 1993/11/23 05:29:18 glass Exp $
 */

#include "systm.h"
#include "param.h"
#include "proc.h"
#include "user.h"

#include "vm/vm.h"

#include "machine/control.h"
#include "machine/cpufunc.h"
#include "machine/cpu.h"
#include "machine/mon.h"
#include "machine/control.h"
#include "machine/pte.h"
#include "machine/pmap.h"
#include "machine/idprom.h"
#include "machine/obio.h"

#include "vector.h"
#include "interreg.h"

unsigned int *old_vector_table;

static struct idprom identity_prom;
unsigned char cpu_machine_id = 0;
char *cpu_string = NULL;

vm_offset_t high_segment_free_start = 0;
vm_offset_t high_segment_free_end = 0;

int msgbufmapped = 0;
struct msgbuf *msgbufp = NULL;
caddr_t vmmap;
extern vm_offset_t tmp_vpages[];
extern int physmem;
unsigned char *interrupt_reg;
unsigned int orig_nmi_vector;
vm_offset_t u_area_va;
vm_offset_t proc0_user_pa;
struct user *proc0paddr;
extern struct pcb *curpcb;

static void initialize_vector_table()
{
    int i;

    mon_printf("initializing vector table (starting)\n");
    old_vector_table = getvbr();
    for (i = 0; i < NVECTORS; i++) {
	if (vector_table[i] == COPY_ENTRY)
	    set_vector_entry(i, old_vector_table[i]);
    }
    setvbr((unsigned int *) vector_table);
    orig_nmi_vector = get_vector_entry(VEC_LEVEL_7_INT);
    mon_printf("initializing vector table (finished)\n");
}

vm_offset_t high_segment_alloc(npages)
     int npages;
{
    int i;
    vm_offset_t va, tmp;

    if (npages == 0)
	mon_panic("panic: request for high segment allocation of 0 pages");
    if (high_segment_free_start == high_segment_free_end) return NULL;

    va = high_segment_free_start + (npages*NBPG);
    if (va > high_segment_free_end) return NULL;
    tmp = high_segment_free_start;
    high_segment_free_start = va;
    return tmp;
}

void sun3_stop()
{
    unsigned int *new_vect;
    mon_printf("sun3_stop: kernel ended deliberately\n");
/*    set_clk_mode(0, IREG_CLOCK_ENAB_5);*/
    mon_printf("sun3_stop: clock(0,0)\n");
    setvbr(old_vector_table);
    new_vect = getvbr();
    mon_printf("post: nmi vec %x\n", new_vect[VEC_LEVEL_7_INT]);
/*    set_clk_mode(IREG_CLOCK_ENAB_7,0);*/
    mon_printf("interrupt_reg_value: %x\n", *interrupt_reg);
    mon_printf("sun3_stop: clock(7,1)\n");
    mon_exit_to_mon();
}

void sun3_printf(str)
     char *str;
{
    mon_printf(str);
}

/*
 * will this actually work or will it have problems because of this supposed
 * wierd thing with (word or short?) aligned addresses
 *
 * needs checksumming support as well
 */

int idprom_fetch(idp, version)
     struct idprom *idp;
     int version;
{
    control_copy_byte(IDPROM_BASE, (caddr_t) idp, sizeof(idp->idp_format));
    if (idp->idp_format != version) return 1;
    control_copy_byte(IDPROM_BASE, (caddr_t) idp, sizeof(struct idprom));
    return 0;
}

void sun3_context_equiv()
{
    unsigned int i, sme;
    int x;
    vm_offset_t va;

    for (x = 1; x < NCONTEXT; x++) {
	for (va = 0; va < (vm_offset_t) (NBSG * NSEGMAP); va += NBSG) {
	    sme = get_segmap(va);
	    mon_setcxsegmap(x, va, sme);
	}
    }
}

void u_area_bootstrap(u_va, u_pa)
     vm_offset_t u_va, u_pa;
{
    vm_offset_t pte_proto, pa, va;

    pte_proto = PG_VALID|PG_WRITE|PG_SYSTEM|PG_NC;

    for (va = u_va, pa = u_pa; va < u_va+NBPG*UPAGES; va+=NBPG, pa+=NBPG)
	set_pte(va, pte_proto|PA_PGNUM(pa));
}

void sun3_vm_init()
{
    unsigned int monitor_memory = 0;
    vm_offset_t va, eva, sva, pte, temp_seg, clock_va;
    extern char start[], etext[], end[];
    unsigned char sme;
    int valid;


    pmeg_init();

    va = (vm_offset_t) start;
    while (va < (vm_offset_t) end) {
	sme = get_segmap(va);
	if (sme == SEGINV)
	    mon_panic("stealing pages for kernel text/data/etc\n");
	pmeg_steal(sme);
	va = sun3_round_up_seg(va);
    }

    virtual_avail = sun3_round_seg(end); /* start a new segment */
    virtual_end = VM_MAX_KERNEL_ADDRESS;

    if (romp->romvecVersion >=1)
	monitor_memory = *romp->memorySize - *romp->memoryAvail;

    mon_printf("%x bytes stolen by monitor\n", monitor_memory);
    
    avail_start = sun3_round_page(end) - KERNBASE; /* XXX */
    avail_end = sun3_trunc_page(*romp->memoryAvail);


    /*
     * preserve/protect monitor: 
     *   need to preserve/protect pmegs used to map monitor between
     *        MONSTART, MONEND.
     *   free up any pmegs in this range which are 
     *   deal with the awful MONSHORTSEG/MONSHORTPAGE
     */
    va = MONSTART; 
    while (va < MONEND) {
	sme = get_segmap(va);
	if (sme == SEGINV) {
	    va = sun3_round_up_seg(va);
	    continue;
	}
	eva = sun3_round_up_seg(va);
	if (eva > MONEND)
	    eva = MONEND;
	valid = 0;
	sva = va;
	for (; va < eva; va += NBPG) {
	    pte = get_pte(va);
	    if (pte & PG_VALID) {
		valid++;
		break;
	    }
	}
	if (valid) 
	    pmeg_steal(sme);
	else {
	    set_segmap(sva, SEGINV);
	}
	va = eva;
    }
    mon_printf("monitor protected\n");
    /*
     * MONSHORTSEG contains MONSHORTPAGE which is some stupid page
     * allocated by the monitor.  One page, in an otherwise empty segment.
     * 
     * basically we are stealing the rest of the segment in hopes of using
     * it to map devices or something.  Really isn't much else you can do with
     * it.  Could put msgbuf's va up there, but i'd prefer if it was in
     * the range of acceptable kernel vm, and not in space.
     *
     */
    
    sme = get_segmap(MONSHORTSEG);
    pmeg_steal(sme);
    high_segment_free_start = MONSHORTSEG;
    high_segment_free_end = MONSHORTPAGE;

    for (va = high_segment_free_start; va < high_segment_free_end;
	 va+= NBPG) 
	set_pte(va, PG_INVAL);
    
    /*
     * unmap user virtual segments
     */

    va = 0;
    while (va < KERNBASE) {	/* starts and ends on segment boundries */
	set_segmap(va, SEGINV);
	va += NBSG;
    }

    /*
     * we need to kill off a segment to handle the stupid non-contexted
     * pmeg 
     *
     */
    temp_seg = sun3_round_seg(virtual_avail);
    set_temp_seg_addr(temp_seg);
    mon_printf("%x virtual bytes lost allocating temporary segment for pmap\n",
	       temp_seg - virtual_avail); 
    set_segmap(temp_seg, SEGINV);
    virtual_avail = temp_seg + NBSG;

    /* allocating page for msgbuf */
    sme = get_segmap(virtual_avail); 
    if (sme == SEGINV)
	mon_printf("bad pmeg encountered while looking for msgbuf page\n");
    pmeg_steal(sme);
    eva = sun3_round_up_seg(virtual_avail);

    /* msgbuf, pg 0 */
    avail_end -= NBPG;
    msgbufp = (struct msgbuf *) virtual_avail;
    pte = PG_VALID | PG_WRITE |PG_SYSTEM | PG_NC | (avail_end >>PGSHIFT);
    set_pte((vm_offset_t) msgbufp, pte);
    msgbufmapped = 1;

    /* cleaning rest of segment */
    virtual_avail +=NBPG;
    for (va = virtual_avail; va < eva; va += NBPG)
	set_pte(va, PG_INVAL);
    
    /* vmmap (used by /dev/mem), pg 1*/
    vmmap = (caddr_t) virtual_avail;
    virtual_avail += NBPG;

    /*
     * vpages array:
     *   just some virtual addresses for temporary mappings
     *   in the pmap module (pg 2-3)
     */

    tmp_vpages[0] = virtual_avail;
    virtual_avail += NBPG;
    tmp_vpages[1] = virtual_avail;
    virtual_avail += NBPG;


    virtual_avail = eva;

    mon_printf("unmapping range: %x to %x\n", virtual_avail, virtual_end);
    /*
     * unmap kernel virtual space (only segments.  if it squished ptes, bad
     * things might happen.
     */

    /* this only works because both are seg bounds*/
    va = virtual_avail;
    while (va < virtual_end) {	
	set_segmap(va, SEGINV);
	va = sun3_round_up_seg(va);
    }

    /* My sincere apologies for this crud -- glass*/
    u_area_va = high_segment_alloc(UPAGES*2);
    if (u_area_va != MONSHORTSEG) /* pg 3,4,5 */
	mon_printf("sun3_vm_init: not at right location for upage\n");
    avail_end -= UPAGES*NBPG;	/* steal the UPAGES for proc0 u-area */
    proc0_user_pa = avail_end;	/* UPAGES physical for proc0 u-area */

    /*
     * first UPAGES are used for u-area standard mapping
     * second UPAGES are used for proc0's personal u-area, and will be mapped
     * to real pages
     *
     * then the standard u-area will be loaded with proc0's u-area
     */
       
    proc0paddr = (struct user *) (u_area_va+UPAGES*NBPG);/* proc0's u-are va */
    /* need to load proc0paddr area with the physical pages stolen before */
    u_area_bootstrap((vm_offset_t) proc0paddr, proc0_user_pa);
    bzero(proc0paddr, UPAGES*NBPG);
    save_u_area(&proc0paddr->u_pcb, proc0paddr);
    load_u_area(&proc0paddr->u_pcb);
    curpcb = &proc0paddr->u_pcb;

    clock_va = (vm_offset_t) obio_alloc((caddr_t) OBIO_CLOCK,
					OBIO_CLOCK_SIZE, OBIO_WRITE);
    if (clock_va != CLOCK_VA)
	mon_printf("clock is not at CLOCK_VA, %x vs. %x\n", clock_va,
		   CLOCK_VA);

    interrupt_reg = obio_alloc((caddr_t) OBIO_INTERREG,
			       OBIO_INTERREG_SIZE, OBIO_WRITE);
    if ((unsigned int) interrupt_reg != INTERREG_VA)
	mon_printf("interrupt_reg is not at INTERREG_VA, %x vs. %x\n",
		   interrupt_reg, INTERREG_VA);
    sun3_context_equiv();
}

void kstack_fall_off()
{
    mon_printf("kstack: fell off\n");
}

void idprom_etheraddr(eaddrp)
     u_char *eaddrp;
{
    eaddrp[0] = identity_prom.idp_etheraddr[0];
    eaddrp[1] = identity_prom.idp_etheraddr[1];
    eaddrp[2] = identity_prom.idp_etheraddr[2];
    eaddrp[3] = identity_prom.idp_etheraddr[3];
    eaddrp[4] = identity_prom.idp_etheraddr[4];
    eaddrp[5] = identity_prom.idp_etheraddr[5];
}

void sun3_verify_hardware()
{
    unsigned char arch;
    int cpu_match = 0;

    if (idprom_fetch(&identity_prom, IDPROM_VERSION))
	mon_panic("idprom fetch failed\n");
    arch = identity_prom.idp_machtype & CPU_ARCH_MASK;
    if (!(arch & SUN3_ARCH))
	mon_panic("not a sun3?\n");
    cpu_machine_id = identity_prom.idp_machtype & SUN3_IMPL_MASK;
    switch (cpu_machine_id) {
    case SUN3_MACH_160:
#ifdef SUN3_160
	cpu_match++;
#endif
	cpu_string = "160";
	break;
    case SUN3_MACH_50 :
#ifdef SUN3_50
	cpu_match++;
#endif
	cpu_string = "50";
	break;
    case SUN3_MACH_260:
#ifdef SUN3_260
	cpu_match++;
#endif
	cpu_string = "260";
	break;
    case SUN3_MACH_110:
#ifdef SUN3_110
	cpu_match++;
#endif
	cpu_string = "110";
	break;
    case SUN3_MACH_60 :
#ifdef SUN3_60
	cpu_match++;
#endif
	cpu_string = "60";
	break;
    case SUN3_MACH_E  :
#ifdef SUN3_E
	cpu_match++;
#endif
	cpu_string = "E";
	break;
    default:
	mon_panic("unknown sun3 model\n");
    }
    if (!cpu_match)
	mon_panic("kernel not configured for the Sun 3 model\n");
    mon_printf("kernel configured for Sun 3/%s\n", cpu_string);
}

/*
 * Print out a traceback for the caller - can be called anywhere
 * within the kernel or from the monitor by typing "g4" (for sun-2
 * compatibility) or "w trace".  This causes the monitor to call
 * the v_handler() routine which will call tracedump() for these cases.
 */
/*VARARGS0*/
tracedump(x1)
	caddr_t x1;
{
    struct funcall_frame *fp = (struct funcall_frame *)(&x1 - 2);
    u_int tospage = btoc(fp);

    mon_printf("Begin traceback...fp = %x\n", fp);
    while (btoc(fp) == tospage) {
	if (fp == fp->fr_savfp) {
	    mon_printf("FP loop at %x", fp);
	    break;
	}
	mon_printf("Called from %x, fp=%x, args=%x %x %x %x\n",
		   fp->fr_savpc, fp->fr_savfp,
		   fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
	fp = fp->fr_savfp;
    }
    mon_printf("End traceback...\n");
}

/*
 * Handler for monitor vector cmd -
 * For now we just implement the old "g0" and "g4"
 * commands and a printf hack.  [lifted from freed cmu mach3 sun3 port]
 */
void
v_handler(addr, str)
	int addr;
	char *str;
{

    switch (*str) {
    case '\0':
	/*
	 * No (non-hex) letter was specified on
	 * command line, use only the number given
	 */
	switch (addr) {
	case 0:			/* old g0 */
	case 0xd:		/* 'd'ump short hand */
	    panic("zero");
	    /*NOTREACHED*/
		
	case 4:			/* old g4 */
	    tracedump();
	    break;

	default:
	    goto err;
	}
	break;

    case 'p':			/* 'p'rint string command */
    case 'P':
	mon_printf("%s\n", (char *)addr);
	break;

    case '%':			/* p'%'int anything a la printf */
	mon_printf(str, addr);
	mon_printf("\n");
	break;

    case 't':			/* 't'race kernel stack */
    case 'T':
	tracedump();
	break;

    case 'u':			/* d'u'mp hack ('d' look like hex) */
    case 'U':
	if (addr == 0xd) {
	    panic("zero");
	} else
	    goto err;
	break;

    default:
    err:
	mon_printf("Don't understand 0x%x '%s'\n", addr, str);
    }
}
void sun3_monitor_hooks()
{
    if (romp->romvecVersion >= 2)
	*romp->vector_cmd = v_handler;
}

void pte_print(pte)
     vm_offset_t pte;
{
    mon_printf("pte: ");
    if (pte & PG_VALID) {
	mon_printf("Valid ");
	if (pte & PG_WRITE)
	    mon_printf("Write ");
	if (pte & PG_WRITE)
	    mon_printf("Write ");
	if (pte & PG_SYSTEM)
	    mon_printf("System ");
	if (pte & PG_NC)
	    mon_printf("Nocache ");
	if (pte & PG_ACCESS)
	    mon_printf("Accessed ");

	if (pte & PG_MOD)
	    mon_printf("Modified ");
	if (pte & PG_TYPE) {
	    vm_offset_t tmp;
	    tmp = pte >> PG_TYPE_SHIFT;
	    if (tmp & PG_OBIO)
		mon_printf("OBIO ");
	    else if (tmp & PG_VME16D)
		mon_printf("VME16D ");
	    else if (tmp & PG_VME32D)
		mon_printf("VME32D ");
	}
	mon_printf(" PA: %x\n", PG_PA(pte));
    }
    else mon_printf("INVALID\n");
}

void set_interrupt_reg(value)
     unsigned int value;
{
    *interrupt_reg = (unsigned char) value;
}
unsigned int get_interrupt_reg()
{
    vm_offset_t pte;
    return (unsigned int) *interrupt_reg;
}

void sun3_bootstrap()
{
    static char hello[] = "hello world";
    int i;
    extern int cold, astpending;

    /*
     * would do bzero of bss here but our bzero only works <64k stuff
     * so we've bailed and done it in locore right before this routine :)
     */

    cold = 1;
    mon_printf("%s\n", hello);
    mon_printf("\nPROM Version: %x\n", romp->romvecVersion);

    sun3_monitor_hooks();

    sun3_verify_hardware();

    initialize_vector_table();	/* point interrupts/exceptions to our table */

    sun3_vm_init();		/* handle kernel mapping problems, etc */
    printf("sun3 vm initialization complete\n");

    pmap_bootstrap();		/*  */
    printf("pmap module bootstrapped\n");

    internal_configure();	/* stuff that can't wait for configure() */
    
    astpending =0;
    printf("calling main()\n");
}
