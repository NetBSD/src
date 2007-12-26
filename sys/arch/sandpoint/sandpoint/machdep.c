/*	$NetBSD: machdep.c,v 1.39.4.1 2007/12/26 19:42:40 ad Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.39.4.1 2007/12/26 19:42:40 ad Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_ipkdb.h"

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
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ksyms.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/bootinfo.h>

#include <powerpc/oea/bat.h>
#include <powerpc/openpic.h>
#include <powerpc/pic/picvar.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <dev/cons.h>
#include <sys/termios.h>

#include "com.h"
#if (NCOM > 0)
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include "com_eumb.h"
#if (NCOM_EUMB > 0)
#include <sandpoint/sandpoint/eumbvar.h>
#endif

#include "pcib.h"
#include "ksyms.h"

char bootinfo[BOOTINFO_MAXSIZE];

void initppc(u_int, u_int, u_int, void *);
void consinit(void);
void sandpoint_bus_space_init(void);
size_t mpc107memsize(void);

#define	OFMEMREGIONS	32
struct mem_region physmemr[OFMEMREGIONS], availmemr[OFMEMREGIONS];

paddr_t avail_end;
struct pic_ops *isa_pic = NULL;
extern int primary_pic;

#if NKSYMS || defined(DDB) || defined(LKM)
extern void *startsym, *endsym;
#endif

#if 1
extern struct consdev kcomcons;
#endif

void
initppc(u_int startkernel, u_int endkernel, u_int args, void *btinfo)
{
	struct btinfo_magic *bi_magic = btinfo;

	if ((unsigned)btinfo != 0 && (unsigned)btinfo < startkernel
	    && bi_magic->magic == BOOTINFO_MAGIC)
		memcpy(bootinfo, btinfo, sizeof(bootinfo));
	else
		args = RB_SINGLE;	/* boot via S-record loader */
		
	/*
	 * Determine real size by analysing SDRAM control registers. 
	 */
	{
		struct btinfo_memory *meminfo;
		size_t memsize;

		meminfo = lookup_bootinfo(BTINFO_MEMORY);
		if (meminfo)
			memsize = meminfo->memsize & ~PGOFSET;
		else
			memsize = mpc107memsize();
		physmemr[0].start = 0;
		physmemr[0].size = memsize;
		availmemr[0].start = (endkernel + PGOFSET) & ~PGOFSET;
		availmemr[0].size = memsize - availmemr[0].start;
	}
	avail_end = physmemr[0].start + physmemr[0].size;    /* XXX temporary */

	/*
	 * Get CPU clock
	 */
	{
		struct btinfo_clock *clockinfo;
		u_long ticks;
		extern u_long ticks_per_sec, ns_per_tick;

		ticks = 1000000000;	/* 100 MHz */
		ticks /= 4;		/* 4 cycles per DEC tick */
		clockinfo = lookup_bootinfo(BTINFO_CLOCK);
		if (clockinfo)
			ticks = clockinfo->ticks_per_sec;
		ticks_per_sec = ticks;
		ns_per_tick = 1000000000 / ticks_per_sec;
	}

	/*
	 * boothowto
	 */
	boothowto = args;

	/*
	 * Now setup fixed bat registers
	 * We setup a pair of BAT to have "Map B" layout, one for the
	 * PCI memory space, another to cover many; MPC107/MPC824x EUMB,
	 * ISA mem, PCI/ISA I/O, PCI configuration, PCI interrupt
	 * acknowledge and flash/ROM space.
	 */
	oea_batinit(
	    0x80000000, BAT_BL_256M,	/* SANDPOINT_BUS_SPACE_MEM */
	    0xfc000000, BAT_BL_64M,	/* _EUMB|_IO */
	    0);

	/* Install vectors and interrupt handler */
	oea_init(NULL);

#if 1 /* bumpy ride in pre-dawn time, for people knows what he/she is doing */
	cn_tab = &kcomcons;
	(*cn_tab->cn_init)(&kcomcons);

	ksyms_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/* Initialize bus_space */
	sandpoint_bus_space_init();

	/* Initialize the console */
	consinit();

	/* Set the page size */
	uvm_setpagesize();

	/* Initialize pmap module */
	pmap_bootstrap(startkernel, endkernel);

#if 0 /* NKSYMS || defined(DDB) || defined(LKM) */
	ksyms_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
#endif
#ifdef IPKDB
	/*
	 * Now trap to IPKDB
	 */
	ipkdb_init();
	if (boothowto & RB_KDB)
		ipkdb_connect(0);
#endif
}

void
mem_regions(struct mem_region **mem, struct mem_region **avail)
{

	*mem = physmemr;
	*avail = availmemr;
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	int msr;
	void *baseaddr;

	/*
	 * Do common startup.
	 */
	oea_startup(NULL);

	/*
	 * Prepare EPIC and install external interrupt handler.
	 *  0..15	used by South bridge i8259 PIC if exists.
	 *  16..39/41	EPIC interrupts, 24 source or 26 source.
	 */
	baseaddr = (void *)(SANDPOINT_BUS_SPACE_EUMB + 0x40000);
	pic_init();
	isa_pic = setup_i8259();
	(void)setup_openpic(baseaddr, 0);
	primary_pic = 1;

#if (NPCIB > 0)
	/*
	 * set up i8259 as a cascade on EPIC irq 0.
	 * XXX exceptional SP2 has 17
	 */
	intr_establish(16, IST_LEVEL, IPL_NONE, pic_handle_intr, isa_pic);
#endif

	oea_install_extint(pic_ext_intr);

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();

	/*
	 * Now allow hardware interrupts.
	 */
	splhigh();
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
	    :	"=r"(msr)
	    :	"K"(PSL_EE));
}

#if (NPCIB > 0)
struct btinfo_console bi_cons = { { 0, 0 },  "com", 0x3f8, 38400 };
#else
struct btinfo_console bi_cons = { { 0, 0 },  "eumb", 0x4600, 57600 };
#endif

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	struct btinfo_common *help = (struct btinfo_common *)bootinfo;

	if (help->next == 0)
		return (NULL);	/* bootinfo[] was not made */ 
	do {
		bt = help;
		if (bt->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + bt->next);
	} while (bt->next &&
		(size_t)help < (size_t)bootinfo + BOOTINFO_MAXSIZE);

	return (NULL);
}

/*
 * consinit
 * Initialize system console.
 */
void
consinit(void)
{
	struct btinfo_console *bi;
	static int initted;

	if (initted)
		return;
	initted = 1;

	bi = lookup_bootinfo(BTINFO_CONSOLE);
	if (bi == NULL)
		bi = &bi_cons;

#if (NCOM > 0)
	if (strcmp(bi->devname, "com") == 0) {
		bus_space_tag_t tag = &genppc_isa_io_space_tag;
		if (comcnattach(tag, bi->addr, bi->speed,
		    COM_FREQ, COM_TYPE_NORMAL,
		    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
			panic("can't init com serial console");
		return;
	}
#endif
#if (NCOM_EUMB > 0)
	if (strcmp(bi->devname, "eumb") == 0) {
		bus_space_tag_t tag = &sandpoint_eumb_space_tag;
		extern u_long ticks_per_sec;

		if (eumbcnattach(tag, bi->addr, bi->speed,
		    4 * ticks_per_sec, COM_TYPE_NORMAL,
		    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
			panic("can't init eumb serial console");
		return;
	}
#endif
	panic("console device missing -- serial console not in kernel");
	/* Of course, this is moot if there is no console... */
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && syncing == 0) {
		syncing = 1; 
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}	    
	
	/* Disable intr */
	splhigh();	
	
	/* Do dump if requested */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		oea_dumpsys();
	
	doshutdownhooks();
	 
	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* for proper keyboard command handling */
		cngetc();  
		cnpollc(0);
	}
    
	printf("rebooting...\n\n");

#if 1
{ extern void sandpoint_reboot(void);
	sandpoint_reboot();
}
#endif
	while (1);
}

struct powerpc_bus_space sandpoint_io_space_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	0xfe000000, 0x00000000, 0x00c00000,
};
struct powerpc_bus_space genppc_isa_io_space_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_IO_TYPE,
	0xfe000000, 0x00000000, 0x00010000,
};
struct powerpc_bus_space sandpoint_mem_space_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0x80000000, 0xfbffffff,
};
struct powerpc_bus_space genppc_isa_mem_space_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0x00000000, 0xfd000000, 0xfe000000,
};
struct powerpc_bus_space sandpoint_eumb_space_tag = {
	_BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE,
	0xfc000000, 0x00000000, 0x00100000,
};

static char ex_storage[5][EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

void
sandpoint_bus_space_init(void)
{
	int error;

	error = bus_space_init(&sandpoint_io_space_tag, "ioport",
	    ex_storage[0], sizeof(ex_storage[0]));
	if (error)
		panic("sandpoint_bus_space_init: can't init io tag");

	error = extent_alloc_region(sandpoint_io_space_tag.pbs_extent,
	    0x10000, 0x7fffff, EX_NOWAIT);
	if (error)
		panic("sandpoint_bus_space_init: can't block out reserved I/O"
		    " space 0x10000-0x7fffff: error=%d", error);

	error = bus_space_init(&sandpoint_mem_space_tag, "iomem",
	    ex_storage[1], sizeof(ex_storage[1]));
	if (error)
		panic("sandpoint_bus_space_init: can't init mem tag");

	error = bus_space_init(&genppc_isa_io_space_tag, "isa-ioport",
	    ex_storage[2], sizeof(ex_storage[2]));
	if (error)
		panic("sandpoint_bus_space_init: can't init isa io tag");

	error = bus_space_init(&genppc_isa_mem_space_tag, "isa-iomem",
	    ex_storage[3], sizeof(ex_storage[3]));
	if (error)
		panic("sandpoint_bus_space_init: can't init isa mem tag");

	error = bus_space_init(&sandpoint_eumb_space_tag, "eumb",
	    ex_storage[4], sizeof(ex_storage[4]));
	if (error)
		panic("sandpoint_bus_space_init: can't init eumb tag");
}

#define MPC107_EUMBBAR		0x78	/* Eumb base address */
#define MPC107_MEMENDADDR1	0x90	/* Memory ending address 1 */
#define MPC107_EXTMEMENDADDR1	0x98	/* Extd. memory ending address 1 */
#define MPC107_MEMEN		0xa0	/* Memory enable */

size_t
mpc107memsize(void)
{
	/*
	 * assumptions here;
	 * - first 4 sets of SDRAM controlling register have been
	 * set right in the ascending order.
	 * - total SDRAM size is the last valid SDRAM address +1.
	 */
	unsigned val, n, bankn, end;

	out32rb(0xfec00000, (1U<<31) | MPC107_MEMEN);
	val = in32rb(0xfee00000);
	if ((val & 0xf) == 0)
		return 32 * 1024 * 1024; /* eeh? */

	bankn = 0;
	for (n = 0; n < 4; n++) {
		if ((val & (1U << n)) == 0)
			break;
		bankn = n;
	}
	bankn = bankn * 8;

	/* the format is "00 xx xxxxxxxx << 20" */
	out32rb(0xfec00000, (1U<<31) | MPC107_EXTMEMENDADDR1); /* bit 29:28 */
	val = in32rb(0xfee00000);
	end =  ((val >> bankn) & 0x03) << 28;
	out32rb(0xfec00000, (1U<<31) | MPC107_MEMENDADDR1);    /* bit 27:20 */
	val = in32rb(0xfee00000);
	end |= ((val >> bankn) & 0xff) << 20;
	end |= 0xfffff;					       /* bit 19:00 */

	return (end + 1); /* recongize this as the amount of SDRAM */
}

/* XXX needs to make openpic.c implementation-neutral XXX */

unsigned epicsteer[] = {
	0x10200,	/* external irq 0 direct/serial */
	0x10220,	/* external irq 1 direct/serial */
	0x10240,	/* external irq 2 direct/serial */
	0x10260,	/* external irq 3 direct/serial */
	0x10280,	/* external irq 4 direct/serial */
	0x102a0,	/* external irq 5 serial mode */
	0x102c0,	/* external irq 6 serial mode */
	0x102e0,	/* external irq 7 serial mode */
	0x10300,	/* external irq 8 serial mode */
	0x10320,	/* external irq 9 serial mode */
	0x10340,	/* external irq 10 serial mode */
	0x10360,	/* external irq 11 serial mode */
	0x10380,	/* external irq 12 serial mode */
	0x103a0,	/* external irq 13 serial mode */
	0x103c0,	/* external irq 14 serial mode */
	0x103e0,	/* external irq 15 serial mode */
	0x11020,	/* I2C */
	0x11040,	/* DMA 0 */
	0x11060,	/* DMA 1 */
	0x110c0,	/* MU/I2O */
	0x01120,	/* Timer 0 */
	0x01160,	/* Timer 1 */
	0x011a0,	/* Timer 2 */
	0x011e0,	/* Timer 3 */
	0x11120,	/* DUART 0, MPC8245 */
	0x11140,	/* DUART 1, MPC8245 */
};

/* XXX XXX debug purpose only XXX XXX */

static dev_type_cninit(kcomcninit);
static dev_type_cngetc(kcomcngetc);
static dev_type_cnputc(kcomcnputc);
static dev_type_cnpollc(kcomcnpollc);

struct consdev kcomcons = {
	NULL, kcomcninit, kcomcngetc, kcomcnputc, kcomcnpollc, NULL,
	NULL, NULL, NODEV, CN_NORMAL
};

#define THR		0
#define RBR		0
#define LSR		5
#define LSR_THRE	0x20
#define UART_READ(r)		*(volatile char *)(uartbase + (r))
#define UART_WRITE(r, v)	*(volatile char *)(uartbase + (r)) = (v)
#define LSR_RFE		0x80
#define LSR_TXEMPTY		0x20
#define LSR_BE			0x10
#define LSR_FE			0x08
#define LSR_PE			0x04
#define LSR_OE			0x02
#define LSR_RXREADY		0x01
#define LSR_ANYE		(LSR_OE|LSR_PE|LSR_FE|LSR_BE)

static unsigned uartbase = 0xfe0003f8;

static void
kcomcninit(struct consdev *cn)
{
	struct btinfo_console *bi = lookup_bootinfo(BTINFO_CONSOLE);

	if (bi == NULL)
		bi = &bi_cons;
	if (strcmp(bi->devname, "com") == 0)
		uartbase = 0xfe000000 + bi->addr;
	else if (strcmp(bi->devname, "eumb") == 0)
		uartbase = 0xfc000000 + bi->addr;
	/*
	 * we do not touch UART operating parameters since bootloader
	 * is supposed to have done well.
	 */
}

static int
kcomcngetc(dev_t dev)
{
	unsigned lsr;
	int s, c;

	s = splserial();
#if 1
	do {
		lsr = UART_READ(LSR);
	} while ((lsr & LSR_ANYE) || (lsr & LSR_RXREADY) == 0);
#else
    again:
	do {
		lsr = UART_READ(LSR);
	} while ((lsr & LSR_RXREADY) == 0);
	if (lsr & (LSR_BE | LSR_FE | LSR_PE)) {
		(void)UART_READ(RBR);
		goto again;
	}
#endif
	c = UART_READ(RBR);
	splx(s);
	return c & 0xff;
}

static void
kcomcnputc(dev_t dev, int c)
{
	unsigned lsr, timo;
	int s;

	s = splserial();
	timo = 150000;
	do {
		lsr = UART_READ(LSR);
	} while (timo-- > 0 && (lsr & LSR_TXEMPTY) == 0);
	if (timo > 0)
		UART_WRITE(THR, c);
	splx(s);
}

static void
kcomcnpollc(dev_t dev, int on)
{
}
