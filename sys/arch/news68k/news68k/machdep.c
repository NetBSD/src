/*	$NetBSD: machdep.c,v 1.139 2026/04/23 02:54:40 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.139 2026/04/23 02:54:40 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_newsconf.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/tty.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/cpu.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif
#ifdef __ELF__
#include <sys/exec_elf.h>
#endif

#include <m68k/cacheops.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/pte.h>
#include <machine/intr.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <m68k/seglist.h>

#include <dev/cons.h>
#include <dev/mm.h>

#define MAXMEM	64*1024		/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <news68k/news68k/machid.h>
#include <news68k/news68k/isr.h>

#include "le.h"
#include "kb.h"
#include "ms.h"
#include "si.h"
#include "ksyms.h"
#include "romcons.h"
/* XXX etc. etc. */

int	maxmem;			/* max memory per process */

extern paddr_t avail_start, avail_end;
extern int end, *esym;
extern u_int lowram;

#ifdef news1700
static void news1700_init(void);
static void parityenable(void);
static int parityerror(void *);
#endif
#ifdef news1200
static void news1200_init(void);
#endif

/* functions called from locore.s */
void machine_init(paddr_t);

int	delay_divisor = delay_divisor_est(25);

#ifdef __HAVE_NEW_PMAP_68K
/*
 * Clamp the kernel virtual address space to keep it out of the
 * TT ranges we use for devices.
 */
const struct pmap_bootmap machine_bootmap[] = {
	{ .pmbm_vaddr = NEWS68K_IO_TT_BASE,
	  .pmbm_size  = NEWS68K_IO_TT_SIZE,
	  .pmbm_flags = PMBM_F_KEEPOUT },

	{ .pmbm_vaddr = NEWS68K_PROM_TT_BASE,
	  .pmbm_size  = NEWS68K_PROM_TT_SIZE,
	  .pmbm_flags = PMBM_F_KEEPOUT },

	{ .pmbm_vaddr = -1 },
};
#endif

/*
 * Early initialization, before main() is called.
 */
void
machine_init(paddr_t nextpa)
{
	/* Clear and enable the external cache, if any. */
	PCIA();
	ecacheon();

	phys_seg_list[0].ps_start = lowram;
	phys_seg_list[0].ps_end = m68k_ptob(maxmem);

	machine_init_common(nextpa);

	/* Initialize system variables. */
	switch (systype) {
#ifdef news1700
	case NEWS1700:
		news1700_init();
		break;
#endif
#ifdef news1200
	case NEWS1200:
		news1200_init();
		break;
#endif
	default:
		panic("impossible system type");
	}
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	/* Initialize the interrupt handlers. */
	isrinit();

#ifdef news1700
	parityenable();
#endif

	cpu_startup_common();
}

int news_machine_id;

void
machine_set_model(void)
{
	/* This has already been done in newsXXXX_init() */
}

void
machine_print_model(void (*pr)(const char *, ...)
		    __printflike(1, 2))
{
	(*pr)("SONY NET WORK STATION, Model %s, Machine ID #%d\n",
	    cpu_getmodel(), news_machine_id);
}

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

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
}

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		cnpollc(true);
		(void)cngetc();
		cnpollc(false);
		printf("\n");
	}
#endif

	/* Finally, halt/reboot the system. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		DELAY(1000000);
		doboot(RB_POWERDOWN);
		/* NOTREACHED */
	}

	if (howto & RB_HALT) {
		printf("System halted.\n\n");
		doboot(RB_HALT);
		/* NOTREACHED */
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot(RB_AUTOBOOT);
	/* NOTREACHED */
}

/* XXX should change the interface, and make one badaddr() function */

int	*nofault;

int
badaddr(void *addr, int nbytes)
{
	int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return 0;
#endif

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return 1;
	}
	switch (nbytes) {
	case 1:
		i = *(volatile char *)addr;
		break;

	case 2:
		i = *(volatile short *)addr;
		break;

	case 4:
		i = *(volatile int *)addr;
		break;

	default:
		panic("badaddr: bad request");
	}
	__USE(i);
	nofault = (int *) 0;
	return 0;
}

int
badbaddr(void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return 1;
	}
	i = *(volatile char *)addr;
	__USE(i);
	nofault = (int *) 0;
	return 0;
}

/*
 *  System dependent initialization
 */

static volatile uint8_t *dip_switch, *int_status;

const uint8_t *idrom_addr;
volatile uint8_t *ctrl_ast, *ctrl_int2;
uint32_t sccport0a, lance_mem_phys;

#ifdef news1700
static volatile u_char *ctrl_parity, *ctrl_parity_clr, *parity_vector;

struct news68k_model {
	const int id;
	const char *name;
};

static const struct news68k_model news68k_models[] = {
	{ ICK001,	"ICK001"	},	/*  1 */
	{ ICK00X,	"ICK00X"	},	/*  2 */
	{ NWS799,	"NWS-799"	},	/*  3 */
	{ NWS800,	"NWS-800"	},	/*  4 */
	{ NWS801,	"NWS-801"	},	/*  5 */
	{ NWS802,	"NWS-802"	},	/*  6 */
	{ NWS711,	"NWS-711"	},	/*  7 */
	{ NWS721,	"NWS-721"	},	/*  8 */
	{ NWS1850,	"NWS-1850"	},	/*  9 */
	{ NWS810,	"NWS-810"	},	/* 10 */
	{ NWS811,	"NWS-811"	},	/* 11 */
	{ NWS1830,	"NWS-1830"	},	/* 12 */
	{ NWS1750,	"NWS-1750"	},	/* 13 */
	{ NWS1720,	"NWS-1720"	},	/* 14 */
	{ NWS1930,	"NWS-1930"	},	/* 15 */
	{ NWS1960,	"NWS-1960"	},	/* 16 */
	{ NWS712,	"NWS-712"	},	/* 17 */
	{ NWS1860,	"NWS-1860"	},	/* 18 */
	{ PWS1630,	"PWS-1630"	},	/* 19 */
	{ NWS820,	"NWS-820"	},	/* 20 */
	{ NWS821,	"NWS-821"	},	/* 21 */
	{ NWS1760,	"NWS-1760"	},	/* 22 */
	{ NWS1710,	"NWS-1710"	},	/* 23 */
	{ NWS830,	"NWS-830"	},	/* 30 */
	{ NWS831,	"NWS-831"	},	/* 31 */
	{ NWS841,	"NWS-841"	},	/* 41 */
	{ PWS1570,	"PWS-1570"	},	/* 52 */
	{ PWS1590,	"PWS-1590"	},	/* 54 */
	{ NWS1520,	"NWS-1520"	},	/* 56 */
	{ PWS1550,	"PWS-1550"	},	/* 73 */
	{ PWS1520,	"PWS-1520"	},	/* 74 */
	{ PWS1560,	"PWS-1560"	},	/* 75 */
	{ NWS1530,	"NWS-1530"	},	/* 76 */
	{ NWS1580,	"NWS-1580"	},	/* 77 */
	{ NWS1510,	"NWS-1510"	},	/* 78 */
	{ NWS1410,	"NWS-1410"	},	/* 81 */
	{ NWS1450,	"NWS-1450"	},	/* 85 */
	{ NWS1460,	"NWS-1460"	},	/* 86 */
	{ NWS891,	"NWS-891"	},	/* 91 */
	{ NWS911,	"NWS-911"	},	/* 111 */
	{ NWS921,	"NWS-921"	},	/* 121 */
	{ 0,		NULL		}
};

static void
news1700_init(void)
{
	struct oidrom idrom;
	const char *t;
	const uint8_t *p;
	uint8_t *q;
	u_int i;

	dip_switch	= (uint8_t *)(0xe1c00100);
	int_status	= (uint8_t *)(0xe1c00200);

	idrom_addr	= (uint8_t *)(0xe1c00000);
	ctrl_ast	= (uint8_t *)(0xe1280000);
	ctrl_int2	= (uint8_t *)(0xe1180000);

	sccport0a	= (0xe0d40002);
	lance_mem_phys	= 0xe0e00000;

	p = idrom_addr;
	q = (uint8_t *)&idrom;

	for (i = 0; i < sizeof(idrom); i++, p += 2)
		*q++ = ((*p & 0x0f) << 4) | (*(p + 1) & 0x0f);

	t = NULL;
	for (i = 0; news68k_models[i].name != NULL; i++) {
		if (news68k_models[i].id == idrom.id_model) {
			t = news68k_models[i].name;
		}
	}
	if (t == NULL)
		panic("unexpected system model.");

	cpu_setmodel("%s", t);
	news_machine_id = (idrom.id_serial[0] << 8) + idrom.id_serial[1];

	ctrl_parity	= (uint8_t *)(0xe1080000);
	ctrl_parity_clr	= (uint8_t *)(0xe1a00000);
	parity_vector	= (uint8_t *)(0xe1c00200);

	cpuspeed_khz = 25*1000;
	delay_divisor = delay_divisor_est(25);
}

/*
 * parity error handling (vectored NMI?)
 */

static void
parityenable(void)
{

	if (systype != NEWS1700)
		return;

#define PARITY_VECT 0xc0
#define PARITY_PRI 7

	*parity_vector = PARITY_VECT;

	isrlink_vectored(parityerror, NULL, PARITY_PRI, PARITY_VECT);

	*ctrl_parity_clr = 1;
	*ctrl_parity = 1;

#ifdef DEBUG
	printf("enable parity check\n");
#endif
}

static int innmihand;	/* simple mutex */

static int
parityerror(void *arg)
{

	/* Prevent unwanted recursion. */
	if (innmihand)
		return 1;
	innmihand = 1;

#if 0 /* XXX need to implement XXX */
	panic("parity error");
#else
	printf("parity error detected.\n");
	*ctrl_parity_clr = 1;
#endif
	innmihand = 0;

	return 1;
}
#endif /* news1700 */

#ifdef news1200
static void
news1200_init(void)
{
	struct idrom idrom;
	const uint8_t *p;
	uint8_t *q;
	int i;

	dip_switch	= (uint8_t *)0xe1680000;
	int_status	= (uint8_t *)0xe1200000;

	idrom_addr	= (uint8_t *)0xe1400000;
	ctrl_ast	= (uint8_t *)0xe1100000;
	ctrl_int2	= (uint8_t *)0xe10c0000;

	sccport0a	= 0xe1780002;
	lance_mem_phys	= 0xe1a00000;

	p = idrom_addr;
	q = (uint8_t *)&idrom;
	for (i = 0; i < sizeof(idrom); i++, p += 2)
		*q++ = ((*p & 0x0f) << 4) | (*(p + 1) & 0x0f);

	cpu_setmodel("%s", idrom.id_model);
	news_machine_id = idrom.id_serial;

	cpuspeed_khz = 25*1000;
	delay_divisor = delay_divisor_est(25);
}
#endif /* news1200 */

/*
 * interrupt handlers
 * XXX should do better handling XXX
 */

void intrhand_lev3(void);
void intrhand_lev4(void);

void
intrhand_lev3(void)
{
	int stat;

	stat = *int_status;
	m68k_count_intr(3);
#if 1
	printf("level 3 interrupt: INT_STATUS = 0x%02x\n", stat);
#endif
}

extern int leintr(int);
extern int si_intr(int);

void
intrhand_lev4(void)
{
	int stat;

#define INTST_LANCE	0x04
#define INTST_SCSI	0x80

	stat = *int_status;
	m68k_count_intr(4);

#if NSI > 0
	if (stat & INTST_SCSI) {
		si_intr(0);
	}
#endif
#if NLE > 0
	if (stat & INTST_LANCE) {
		leintr(0);
	}
#endif
#if 0
	printf("level 4 interrupt\n");
#endif
}

/*
 * consinit() routines - from newsmips/cpu_cons.c
 */

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 * XXX need something better here.
 */
#define SCC_CONSOLE	0
#define SW_CONSOLE	0x07
#define SW_NWB512	0x04
#define SW_NWB225	0x01
#define SW_FBPOP	0x02
#define SW_FBPOP1	0x06
#define SW_FBPOP2	0x03
#define SW_AUTOSEL	0x07

extern struct consdev consdev_rom, consdev_zs;

int tty00_is_console = 0;

void
consinit(void)
{
	uint8_t dipsw;

	dipsw = *dip_switch;

	dipsw = ~dipsw;

	switch (dipsw & SW_CONSOLE) {
	default: /* XXX no real fb support yet */
#if NROMCONS > 0
		cn_tab = &consdev_rom;
		(*cn_tab->cn_init)(cn_tab);
		break;
#endif
	case 0:
		tty00_is_console = 1;
		cn_tab = &consdev_zs;
		(*cn_tab->cn_init)(cn_tab);
		break;
	}
#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)esym - (int)&end - sizeof(Elf32_Ehdr),
	    (void *)&end, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{

	*handled = false;
	if ((uint8_t *)ptr >= intiobase)
		return EFAULT;
	return 0;
}
