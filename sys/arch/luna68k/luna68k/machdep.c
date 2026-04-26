/* $NetBSD: machdep.c,v 1.133 2026/04/26 12:49:38 thorpej Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.133 2026/04/26 12:49:38 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"
#include "opt_panicbutton.h"
#include "opt_m68k_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kauth.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#ifdef	KGDB
#include <sys/kgdb.h>
#endif
#include <sys/boot_flag.h>
#include <sys/exec_elf.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/bootinfo.h>
#include <machine/board.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <m68k/seglist.h>

#include <luna68k/dev/siottyvar.h>

#include <dev/cons.h>
#include <dev/mm.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include "ksyms.h"

int	maxmem;			/* max memory per process */

extern	u_int lowram;

void machine_init(paddr_t);

void nmihand(struct frame);

#if NKSYMS || defined(DDB) || defined(MODULAR)
vsize_t symtab_size(vaddr_t);
#endif
extern char end[];
extern void *esym;

int	machtype;	/* model: 1 for LUNA-1, 2 for LUNA-2 */
int	sysconsole;	/* console: 0 for ttya, 1 for video */

extern void omfb_cnattach(void);
extern void ws_cnattach(void);

/*
 * Default the delay_divisor to 25MHz 68040.
 */
int	delay_divisor = delay_divisor_est40(25);

/*
 * Clamp the kernel virtual address space to keep it out of the
 * TT ranges we use for devices.
 */
struct pmap_bootmap machine_bootmap[] = {
	{ .pmbm_vaddr = LUNA68K_IO0_TT_BASE,
	  .pmbm_size  = LUNA68K_IO0_TT_SIZE,
	  .pmbm_flags = PMBM_F_KEEPOUT },

	{ .pmbm_vaddr = LUNA68K_IO1_TT_BASE,
	  .pmbm_size  = LUNA68K_IO1_TT_SIZE,
	  .pmbm_flags = PMBM_F_KEEPOUT },

	{ .pmbm_vaddr = -1 },
};

/*
 * Early initialization, before main() is called.
 */
void
machine_init(paddr_t nextpa)
{
	volatile uint8_t *pio0 = (void *)OBIO_PIO0_BASE;
	int sw1;
	char *cp;
	extern char bootarg[64];

	/* initialize cn_tab for early console */
#if 1
	cn_tab = &siottycons;
#else
	cn_tab = &romcons;
#endif

	phys_seg_list[0].ps_start = 0;	/* XXX lowram? */
	phys_seg_list[0].ps_end = m68k_ptob(maxmem);

	machine_init_common(nextpa);

	pio0[3] = 0xb6;
	pio0[2] = 1 << 6;		/* enable parity check */
	pio0[3] = 0xb6;
	sw1 = pio0[0];			/* dip sw1 value */
	sw1 ^= 0xff;
	sysconsole = !(sw1 & 0x2);	/* console selection */

	/*
	 * Check if boothowto and bootdev values are passed by our bootloader.
	 */
	if ((bootdev & B_MAGICMASK) == B_DEVMAGIC) {
		/* Valid value is set; no need to parse bootarg. */
		return;
	}

	/*
	 * No valid bootdev value is set.
	 * Assume we are booted by ROM monitor directly using a.out kernel
	 * and we have to parse bootarg passed from the monitor to set
	 * proper boothowto and check netboot.
	 */

	/* set default to "sd0a" with no howto flags */
	bootdev = MAKEBOOTDEV(0, LUNA68K_BOOTADPT_SPC, 0, 0, 0);
	boothowto = 0;

	/*
	 * 'bootarg' on LUNA has:
	 *   "<args of x command> ENADDR=<addr> HOST=<host> SERVER=<name>"
	 * where <addr> is MAC address of which network loader used (not
	 * necessarily same as one at 0x4101.FFE0), <host> and <name>
	 * are the values of HOST and SERVER environment variables.
	 *
	 * 'bootarg' on LUNA-II has "<args of x command>" only.
	 *
	 * NetBSD/luna68k cares only the first argument; any of "sda".
	 */
	bootarg[63] = '\0';
	for (cp = bootarg; *cp != '\0'; cp++) {
		if (*cp == '-') {
			char c;
			while ((c = *cp) != '\0' && c != ' ') {
				BOOT_FLAG(c, boothowto);
				cp++;
			}
		} else if (*cp == 'E' && memcmp("ENADDR=", cp, 7) == 0) {
			bootdev =
			    MAKEBOOTDEV(0, LUNA68K_BOOTADPT_LANCE, 0, 0, 0);
		}
	}
}

/*
 * Console initialization: called early on from main,
 */
void
consinit(void)
{

	if (sysconsole == 0) {
		cn_tab = &siottycons;
		(*cn_tab->cn_init)(cn_tab);
	} else {
		omfb_cnattach();
		ws_cnattach();
	}

#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((esym != NULL) ? 1 : 0, (void *)&end, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		cpu_Debugger();
#endif
}

#if NKSYMS || defined(DDB) || defined(MODULAR)

/*
 * Check and compute size of DDB symbols and strings.
 *
 * Note this function could be called from locore.s before MMU is turned on
 * so we should avoid global variables and function calls.
 */
vsize_t
symtab_size(vaddr_t hdr)
{
	int i;
	Elf_Ehdr *ehdr;
	Elf_Shdr *shp;
	vaddr_t maxsym;

	/*
	 * Check the ELF headers.
	 */

	ehdr = (void *)hdr;
	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
	    ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
		return 0;
	}

	/*
	 * Find the end of the symbols and strings.
	 */

	maxsym = 0;
	shp = (Elf_Shdr *)(hdr + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (shp[i].sh_type != SHT_SYMTAB &&
		    shp[i].sh_type != SHT_STRTAB) {
			continue;
		}
		maxsym = uimax(maxsym, shp[i].sh_offset + shp[i].sh_size);
	}

	return maxsym;
}
#endif /* NKSYMS || defined(DDB) || defined(MODULAR) */

void
machine_set_model(void)
{
	switch (cputype) {
#ifdef M68030
	case CPU_68030:
		cpu_setmodel("LUNA-I");
		machtype = LUNA_I;
		/* 20MHz 68030 */
		cpuspeed_khz = 20*1000;
		delay_divisor = delay_divisor_est(20);
		hz = 60;
		break;
#endif
#ifdef M68040
	case CPU_68040:
		cpu_setmodel("LUNA-II");
		machtype = LUNA_II;
		/* 25MHz 68040 */
		cpuspeed_khz = 25*1000;
		delay_divisor = delay_divisor_est40(25);
		/* hz = 100 on LUNA-II */
		break;
#endif
	default:
		panic("unknown CPU type");
	}
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
	extern void doboot(void);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	/* If system is hold, just halt. */
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

	/* Finally, halt/reboot the system. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		volatile uint8_t *pio = (void *)OBIO_PIO1_BASE;

		printf("power is going down.\n");
		DELAY(100000);
		pio[3] = 0x94;
		pio[2] = 0 << 4;
		for (;;)
			/* NOP */;
	}
	if (howto & RB_HALT) {
		printf("System halted.	Hit any key to reboot.\n\n");
		cnpollc(true);
		(void)cngetc();
		cnpollc(false);
	}

	printf("rebooting...\n");
	DELAY(100000);
	doboot();
	/*NOTREACHED*/
	for (;;)
		;
}

void luna68k_abort(const char *);

static int innmihand;	/* simple mutex */

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
void
nmihand(struct frame frame)
{

	/* Prevent unwanted recursion */
	if (innmihand)
		return;
	innmihand = 1;

	luna68k_abort("ABORT SWITCH");

	innmihand = 0;
}

/*
 * Common code for handling ABORT signals from buttons, switches,
 * serial lines, etc.
 */
void
luna68k_abort(const char *cp)
{

#ifdef DDB
	printf("%s\n", cp);
	cpu_Debugger();
#else
#ifdef PANICBUTTON
	panic(cp);
#else
	printf("%s ignored\n", cp);
#endif /* PANICBUTTON */
#endif /* DDB */
}

#ifdef notyet
/*
 * romcons is useful until m68k TC register is initialized.
 */
int  romcngetc(dev_t);
void romcnputc(dev_t, int);

struct consdev romcons = {
	NULL,
	NULL,
	romcngetc,
	romcnputc,
	nullcnpollc,
	makedev(7, 0), /* XXX */
	CN_DEAD,
};

#define __		((int **)PROM_ADDR)
#define GETC()		(*(int (*)())__[6])()
#define PUTC(x)		(*(void (*)())__[7])(x)

#define ROMPUTC(x) \
({					\
	register _r;			\
	__asm volatile ("			\
		movc	%%vbr,%0	; \
		movel	%0,%%sp@-	; \
		clrl	%0		; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	PUTC(x);			\
	__asm volatile ("			\
		movel	%%sp@+,%0	; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
})

#define ROMGETC() \
({					\
	register _r, _c;		\
	__asm volatile ("			\
		movc	%%vbr,%0	; \
		movel	%0,%%sp@-	; \
		clrl	%0		; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	_c = GETC();			\
	__asm volatile ("			\
		movel	%%sp@+,%0	; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	_c;				\
})

void
romcnputc(dev_t dev, int c)
{
	int s;

	s = splhigh();
	ROMPUTC(c);
	splx(s);
}

int
romcngetc(dev_t dev)
{
	int s, c;

	do {
		s = splhigh();
		c = ROMGETC();
		splx(s);
	} while (c == -1);
	return c;
}
#endif
