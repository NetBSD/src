/*	$NetBSD: machdep.c,v 1.8 2009/11/27 03:23:13 rmind Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.8 2009/11/27 03:23:13 rmind Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <net/netisr.h>

#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <machine/iplcb.h>

#include <powerpc/oea/bat.h>
#include <powerpc/pio.h>
#include <arch/powerpc/pic/picvar.h>

#include <dev/cons.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
void comsoft(void);
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include "ksyms.h"

void initppc(u_long, u_long, u_int, void *);
void dumpsys(void);
void strayintr(int);
int lcsplx(int);
void rs6000_bus_space_init(void);
void setled(uint32_t);
void say_hi(void);
static void init_intr(void);

char bootinfo[BOOTINFO_MAXSIZE];
char bootpath[256];
struct ipl_directory	*ipldir;
struct ipl_cb		*iplcb;
struct ipl_info		*iplinfo;
int led_avail;
struct sys_info		*sysinfo;
struct buc_info		*bucinfo[MAX_BUCS];
int nrofbucs;

struct pic_ops *pic_iocc;

#define	OFMEMREGIONS	32
struct mem_region physmemr[OFMEMREGIONS], availmemr[OFMEMREGIONS];

paddr_t avail_end;			/* XXX temporary */
extern register_t iosrtable[16];

#if NKSYMS || defined(DDB) || defined(MODULAR)
extern void *endsym, *startsym;
#endif

#ifndef CONCOMADDR
#define CONCOMADDR 0x30
#endif

#ifndef CONSPEED
#define CONSPEED 9600
#endif

#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;
struct consdev kcomcons;
static void kcomcnputc(dev_t, int);

extern struct pic_ops *setup_iocc(void);


void
say_hi(void)
{
	printf("HELLO?!\n");
	setled(0x55500000);
#ifdef DDB
	Debugger();
#endif
#if 0
        li      %r28,0x00000041 /* PUT A to R28*/         
        li      %r29,0x30       /* put serial addr to r29*/
        addis   %r29,%r29,0xc000 /* r29 now holds serial addr*/
        stb     %r28,0(%r29)  /* slam it to serial port*/        
loopforever:
        bla     loopforever
#endif
}
/*
 * Set LED's.  Yes I know this is ugly. Yes I will rewrite it in pure asm.
 */
void
setled(uint32_t val)
{
#if 0
	register_t savemsr, msr, savesr15;

	__asm volatile ("mfmsr %0" : "=r"(savemsr));
	msr = savemsr & ~PSL_DR;
	__asm volatile ("mtmsr %0" : : "r"(msr));

	__asm volatile ("mfsr %0,15;isync" : "=r"(savesr15));
	__asm volatile ("mtsr 15,%0" : : "r"(0x82040080));
	__asm volatile ("mtmsr %0" : : "r"(msr|PSL_DR));
	__asm volatile ("isync");
	*(uint32_t *)0xF0A00300 = val;
	__asm volatile ("mtmsr %0" : : "r"(savemsr));
	__asm volatile ("mtsr 15,%0;isync" : : "r"(savesr15));
#endif
	if (led_avail)
		*(uint32_t *)0xFF600300 = val;
}

#if 0
int
power_cputype(void)
{
	uint32_t model;

	model = iplinfo->model;
	if (model & 0x08000000) {
		/* ppc */
		return 0;
	} else if (model & 0x02000000)
		return POWER_RSC;
	else if (model & 0x04000000)
		return POWER_2;
	else
		return POWER_1;
}
#endif


void
initppc(u_long startkernel, u_long endkernel, u_int args, void *btinfo)
{
	u_long ekern;
	struct ipl_cb *r_iplcb;
	struct ipl_directory *r_ipldir;
	struct buc_info *bi;
	int i;
	register_t savemsr, msr;

	/* indicate we have control */
	//setled(0x40000000);
	*(uint8_t *)0xe0000030 = '1';

	/* copy bootinfo */
	memcpy(bootinfo, btinfo, sizeof(bootinfo));
	*(uint8_t *)0xe0000030 = '2';

	/* copy iplcb data */
	{
		struct btinfo_iplcb *iplcbinfo;

		iplcbinfo =
		    (struct btinfo_iplcb *)lookup_bootinfo(BTINFO_IPLCB);
		if (!iplcbinfo)
			panic("no iplcb information found in bootinfo");

		/* round_page endkernel, copy down to there, adjust */
		ekern = round_page(endkernel);

		r_iplcb = (struct ipl_cb *)iplcbinfo->addr;
		r_ipldir = (struct ipl_directory *)&r_iplcb->dir;
		memcpy((void *)ekern, r_iplcb, r_ipldir->cb_bitmap_size);
		iplcb = (struct ipl_cb *)ekern;
		ipldir = (struct ipl_directory *)&iplcb->dir;
		iplinfo = (struct ipl_info *)((char *)iplcb +
		    ipldir->iplinfo_off);
		sysinfo = (struct sys_info *)((char *)iplcb +
		    ipldir->sysinfo_offset);

		ekern += r_ipldir->cb_bitmap_size;
		endkernel = ekern;

	}

	/* IPLCB copydown successful */
	setled(0x40100000);

	/* Set memory region */
	{
		u_long memsize = iplinfo->ram_size;

		/*
		 * Some machines incorrectly report memory size in
		 * MB.  Stupid stupid IBM breaking thier own spec.
		 * on conformant machines, it is:
		 * The highest addressable real memory address byte+1
		 */
		if (memsize < 4069)
			memsize = memsize * 1024 * 1024;
		else
			memsize -= 1;
		physmemr[0].start = 0;
		physmemr[0].size = memsize & ~PGOFSET;
		availmemr[0].start = round_page(endkernel);
		availmemr[0].size = memsize - availmemr[0].start;
	}
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */
	*(uint8_t *)0xe0000030 = '3';

#ifdef NOTYET
	/* hrmm.. there is no timebase crap on POWER */
	/*
	 * Set CPU clock
	 */
	{
		struct btinfo_clock *clockinfo;
		extern u_long ticks_per_sec, ns_per_tick;

		clockinfo =
		    (struct btinfo_clock *)lookup_bootinfo(BTINFO_CLOCK);
		if (!clockinfo)
			panic("not found clock information in bootinfo");

		ticks_per_sec = clockinfo->ticks_per_sec;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}
#endif
	/* boothowto */
	boothowto = args;

	/*
	 * Now setup fixed bat registers
	 * 1) POWER has no bat registers.
	 * 2) NVRAM, IPL ROM, and other fun bits all live at 0xFF000000 on the
	 *    60x RS6000's, however if you try to map any less than 256MB
	 *    on a 601 it wedges. 
	 */
#if !defined(POWER)
	setled(0x40200000);
	oea_batinit(
	    0xF0000000, BAT_BL_256M,
/*
	    RS6000_BUS_SPACE_MEM, BAT_BL_256M,
	    RS6000_BUS_SPACE_IO,  BAT_BL_256M,
	    0xbf800000, BAT_BL_8M,
*/
	    0);
	led_avail = 1;
#endif
	setled(0x40200000);

	/* set the IO segreg for the first IOCC */
#ifdef POWER
	iosrtable[0xc] = 0x820C00E0;
#else
	iosrtable[0xc] = 0x82000080;
#endif
	__asm volatile ("mfmsr %0" : "=r"(savemsr));
	msr = savemsr & ~PSL_DR;
	__asm volatile ("mtmsr %0" : : "r"(msr));
	__asm volatile ("mtsr 0xc,%0" : : "r"(iosrtable[0xc]));
	__asm volatile ("mtmsr %0" : : "r"(msr|PSL_DR));
	__asm volatile ("isync");
	__asm volatile ("mtmsr %0;isync" : : "r"(savemsr));

	cn_tab = &kcomcons;
	printf("\nNetBSD/rs6000 booting ...\n");
	setled(0x40300000);

	/* Install vectors and interrupt handler. */
	oea_init(NULL);
	setled(0x40400000);

	/* Initialize pmap module. */
	uvm_setpagesize();
	pmap_bootstrap(startkernel, endkernel);
	setled(0x40500000);

	/* populate the bucinfo stuff now that we can malloc */
	bi = (struct buc_info *)((char *)iplcb + ipldir->bucinfo_off);
	nrofbucs = bi->nrof_structs;
	/*printf("nrof=%d\n", nrofbucs);*/
	if (nrofbucs > MAX_BUCS)
		aprint_error("WARNING: increase MAX_BUCS to at least %d\n",
		    nrofbucs);
	for (i=0; i < nrofbucs && i < MAX_BUCS; i++) {
#ifdef DEBUG
		printf("bi addr= %p\n", &bi);
		printf("i=%d ssize=%x\n", i, bi->struct_size);
#endif
		bucinfo[i] = bi;
		bi = (struct buc_info *)((char *)iplcb + ipldir->bucinfo_off +
		    bi->struct_size);
	}
#ifdef DEBUG
	printf("found %d bucs\n", nrofbucs);
	for (i=0; i < nrofbucs; i++) {
		printf("BUC type: %d\n", bucinfo[i]->dev_type);
		printf("BUC cfg incr: %x\n", bucinfo[i]->cfg_addr_incr);
		printf("BUC devid reg: %x\n", bucinfo[i]->device_id_reg);
		printf("BUC iocc?= %d\n", bucinfo[i]->IOCC_flag);
		printf("BUC location= %x%x%x%x\n", bucinfo[i]->location[0],
		    bucinfo[i]->location[1], bucinfo[i]->location[2],
		    bucinfo[i]->location[3]);
	}
	printf("sysinfo scr_addr %p -> %x\n", sysinfo->scr_addr,
	    *sysinfo->scr_addr);
#endif
	
	/* Initialize bus_space. */
	rs6000_bus_space_init();
	setled(0x40600000);

	/* Initialize the console */
	consinit();
	setled(0x41000000);

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)((u_long)endsym - (u_long)startsym), startsym, endsym);
#endif

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{

	*mem = physmemr;
	*avail = availmemr;
}

static void
init_intr(void)
{
	pic_init();
	pic_iocc = setup_iocc();
	oea_install_extint(pic_ext_intr);
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	/* 420 indicates we are entering cpu_startup() */
	setled(0x42000000);
	/* Do common startup. */
	oea_startup("rs6000");

	/*
	 * Inititalize the IOCC interrupt stuff
	 */
	init_intr();

	/*
	 * Now allow hardware interrupts.
	 */
	{
		int msr;

		splraise(-1);
		__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
		setled(0x42200000);
	}
	/*
	 * Now safe for bus space allocation to use malloc.
	 */
	bus_space_mallocok();
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	struct btinfo_common *help = (struct btinfo_common *)bootinfo;

	do {
		bt = help;
		if (bt->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + bt->next);
	} while (bt->next &&
		(size_t)help < (size_t)bootinfo + sizeof (bootinfo));

	return (NULL);
}

/*
 * Reboot an RS6K
 */
static void
reset_rs6000(void)
{
	mtmsr(mfmsr() | PSL_IP);

	/* writing anything to the power/reset reg on an rs6k will cause
	 * a soft reboot. Supposedly.
	 */
	if (sysinfo->prcr_addr)
		outb(sysinfo->prcr_addr, 0x1);
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;

	if (cold) {
		howto |= RB_HALT;
		goto halt_sys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && syncing == 0) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		//resettodr();		/* set wall clock */
	}

	/* Disable intr */
	splhigh();

	/* Do dump if requested */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		oea_dumpsys();

halt_sys:
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if (howto & RB_HALT) {
                printf("\n");
                printf("The operating system has halted.\n");
                printf("Please press any key to reboot.\n\n");
                cnpollc(1);	/* for proper keyboard command handling */
                cngetc();
                cnpollc(0);
	}

	printf("rebooting...\n\n");

	reset_rs6000();

	for (;;)
		continue;
	/* NOTREACHED */
}

/* The iocc0 mapping is set by init_ppc to 0xC via an iosegreg */
struct powerpc_bus_space rs6000_iocc0_io_space_tag = {
	.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	.pbs_offset = 0xC0000000,
	.pbs_base = 0x00000000,
	.pbs_limit = 0x10000000,
};

static char ex_storage[2][EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

void
rs6000_bus_space_init(void)
{
	int error;

	error = bus_space_init(&rs6000_iocc0_io_space_tag, "ioport",
	    ex_storage[0], sizeof(ex_storage[0]));
	if (error)
		panic("rs6000_bus_space_init: can't init io tag");

#if 0
	error = extent_alloc_region(rs6000_iocc0_io_space_tag.pbs_extent,
	    0x10000, 0x7F0000, EX_NOWAIT);
	if (error)
		panic("rs6000_bus_space_init: can't block out reserved I/O"
		    " space 0x10000-0x7fffff: error=%d", error);
	error = bus_space_init(&prep_mem_space_tag, "iomem",
	    ex_storage[1], sizeof(ex_storage[1]));
	if (error)
		panic("prep_bus_space_init: can't init mem tag");

	rs6000_iocc0_io_space_tag.pbs_extent = prep_io_space_tag.pbs_extent;
	error = bus_space_init(&prep_isa_io_space_tag, "isa-ioport", NULL, 0);
	if (error)
		panic("prep_bus_space_init: can't init isa io tag");

	prep_isa_mem_space_tag.pbs_extent = prep_mem_space_tag.pbs_extent;
	error = bus_space_init(&prep_isa_mem_space_tag, "isa-iomem", NULL, 0);
	if (error)
		panic("prep_bus_space_init: can't init isa mem tag");
#endif
}

static bus_space_handle_t kcom_base = (bus_space_handle_t) (0xc0000000 + CONCOMADDR);
extern void bsw1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o, u_int8_t v);
extern int bsr1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o);
#define KCOM_GETBYTE(r)         bsr1(0, kcom_base, (r))
#define KCOM_PUTBYTE(r,v)       bsw1(0, kcom_base, (r), (v))

static int
kcomcngetc(dev_t dev)
{
        int stat, c;
	register_t msr;

	msr = mfmsr();
	mtmsr(msr|PSL_DR);
	__asm volatile ("isync");

        /* block until a character becomes available */
        while (!ISSET(stat = KCOM_GETBYTE(com_lsr), LSR_RXRDY))
                ;

        c = KCOM_GETBYTE(com_data);
        stat = KCOM_GETBYTE(com_iir);
	mtmsr(msr);
	__asm volatile ("isync");
        return c;
}

/*
 * Console kernel output character routine.
 */
static void
kcomcnputc(dev_t dev, int c)
{
        int timo;
	register_t msr;

	msr = mfmsr();
	mtmsr(msr|PSL_DR);
	__asm volatile ("isync");
        /* wait for any pending transmission to finish */
        timo = 150000;
        while (!ISSET(KCOM_GETBYTE(com_lsr), LSR_TXRDY) && --timo)
                continue;

        KCOM_PUTBYTE(com_data, c);

        /* wait for this transmission to complete */
        timo = 1500000;
        while (!ISSET(KCOM_GETBYTE(com_lsr), LSR_TXRDY) && --timo)
                continue;
	mtmsr(msr);
	__asm volatile ("isync");
}

static void
kcomcnpollc(dev_t dev, int on)
{
}

struct consdev kcomcons = {
        NULL, NULL, kcomcngetc, kcomcnputc, kcomcnpollc, NULL,
        NULL, NULL, NODEV, CN_NORMAL
};
