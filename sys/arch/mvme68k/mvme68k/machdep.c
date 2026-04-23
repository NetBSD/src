/*	$NetBSD: machdep.c,v 1.192 2026/04/23 02:54:40 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.192 2026/04/23 02:54:40 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_panicbutton.h"
#include "opt_m68k_arch.h"
#include "opt_mvmeconf.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <sys/exec_elf.h>
#endif

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#define _M68K_BUS_DMA_PRIVATE
#include <machine/bus.h>
#undef _M68K_BUS_DMA_PRIVATE
#include <machine/pcb.h>
#include <machine/prom.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <m68k/include/cacheops.h>
#include <dev/cons.h>
#include <dev/mm.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <mvme68k/dev/mainbus.h>
#include <m68k/seglist.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#endif

#define	MAXMEM	64*1024	/* XXX - from cmap.h */

/*
 * Model information, filled in by the Bug; see locore.s
 */
struct	mvmeprom_brdid  boardid;

/*
 * The driver for the ethernet chip appropriate to the
 * platform (lance or i82586) will use this variable
 * to size the chip's packet buffer.
 */
#ifndef ETHER_DATA_BUFF_PAGES
#define	ETHER_DATA_BUFF_PAGES	4
#endif
u_long	ether_data_buff_size = ETHER_DATA_BUFF_PAGES * PAGE_SIZE;
uint8_t	mvme_ea[6];

extern	u_int lowram;

/*
 * Default delay_divisor to the "worst case" 60MHz 68060.
 */
int	delay_divisor = delay_divisor_est60(60);

/* Machine-dependent initialization routines. */
void	machine_init(paddr_t);

#ifdef MVME147
#include <mvme68k/dev/pccreg.h>
void	mvme147_init(void);
#endif

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
#include <dev/mvme/pcctworeg.h>
void	mvme1xx_init(void);
#endif

/*
 * Early initialization, right before main is called.
 */
void
machine_init(paddr_t nextpa)
{
	extern paddr_t msgbufpa;

	/*
	 * Since mvme68k boards can have anything from 4MB of onboard RAM, we
	 * would rather set the pager_map_size at runtime based on the amount
	 * of onboard RAM.
	 *
	 * Set pager_map_size to half the size of onboard RAM, up to a
	 * maximum of 16MB.
	 * (Note: Just use ps_end here since onboard RAM starts at 0x0)
	 */
	pager_map_size = phys_seg_list[0].ps_end / 2;
	if (pager_map_size > (16 * 1024 * 1024))
		pager_map_size = 16 * 1024 * 1024;

	/*
	 * Put the kernel message buffer at the end of on-board RAM;
	 * VME memory has to be zero'd to initialize parity.
	 *
	 * (This means we have to fully initialize the first phys seg
	 * entry.)
	 */
	phys_seg_list[0].ps_start = phys_seg_list[0].ps_avail_start =
	    m68k_round_page(phys_seg_list[0].ps_start);
	phys_seg_list[0].ps_end = phys_seg_list[0].ps_avail_end =
	    m68k_trunc_page(phys_seg_list[0].ps_end);

	phys_seg_list[0].ps_avail_end -= m68k_round_page(MSGBUFSIZE);
	msgbufpa = phys_seg_list[0].ps_avail_end;

	machine_init_common(nextpa);

	switch (machineid) {
#ifdef MVME147
	case MVME_147:
		mvme147_init();
		break;
#endif
#ifdef MVME167
	case MVME_167:
#endif
#ifdef MVME162
	case MVME_162:
#endif
#ifdef MVME177
	case MVME_177:
#endif
#ifdef MVME172
	case MVME_172:
#endif
#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
		mvme1xx_init();
		break;
#endif
	default:
		panic("%s: impossible machineid", __func__);
	}
}

#ifdef MVME147
/*
 * MVME-147 specific initialization.
 */
void
mvme147_init(void)
{
	bus_space_tag_t bt = &_mainbus_space_tag;
	bus_space_handle_t bh;

	/*
	 * Set up a temporary mapping to the PCC's registers
	 */
	bus_space_map(bt, intiobase_phys + MAINBUS_PCC_OFFSET, PCCREG_SIZE, 0,
	    &bh);

	/*
	 * calibrate delay() using the 6.25 usec counter.
	 * we adjust the delay_divisor until we get the result we want.
	 */
	bus_space_write_1(bt, bh, PCCREG_TMR1_CONTROL, PCC_TIMERCLEAR);
	bus_space_write_2(bt, bh, PCCREG_TMR1_PRELOAD, 0);
	bus_space_write_1(bt, bh, PCCREG_TMR1_INTR_CTRL, 0);

	/*
	 * See delay_divisor_est() definition and recommendation to
	 * assume a bit slower than you'll actually see.
	 */
	for (delay_divisor = delay_divisor_est(delay_calibration_weight(16));
	     delay_divisor > 0; delay_divisor--) {
		bus_space_write_1(bt, bh, PCCREG_TMR1_CONTROL, PCC_TIMERSTART);
		delay(10000);
		bus_space_write_1(bt, bh, PCCREG_TMR1_CONTROL, PCC_TIMERSTOP);

		/* 1600 * 6.25usec == 10000usec */
		if (bus_space_read_2(bt, bh, PCCREG_TMR1_COUNT) > 1600)
			break;	/* got it! */

		bus_space_write_1(bt, bh, PCCREG_TMR1_CONTROL, PCC_TIMERCLEAR);
		/* retry! */
	}
	/* just in case */
	if (delay_divisor == 0) {
		delay_divisor = 1;
	}

	bus_space_unmap(bt, bh, PCCREG_SIZE);

	/* calculate cpuspeed */
	cpuspeed_khz = delay_divisor_est(delay_divisor) * 1000;
}
#endif /* MVME147 */

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
int	get_cpuspeed(void);

/*
 * MVME-1[67]x specific initialization.
 */
void
mvme1xx_init(void)
{
	bus_space_tag_t bt = &_mainbus_space_tag;
	bus_space_handle_t bh;

	/*
	 * Set up a temporary mapping to the PCCChip2's registers
	 */
	bus_space_map(bt,
	    intiobase_phys + MAINBUS_PCCTWO_OFFSET + PCCTWO_REG_OFF,
	    PCC2REG_SIZE, 0, &bh);

	bus_space_write_1(bt, bh, PCC2REG_TIMER1_ICSR, 0);

	/*
	 * See delay_divisor_est() definition and recommendation to
	 * assume a bit slower than you'll actually see.
	 */
	if (cputype == CPU_68060) {
		/* MVME-177 came in a 50MHz variant. */
		delay_divisor =
		    delay_divisor_est60(delay_calibration_weight(50));
	} else {
		/* MVME-162 came in 25Hz variant. */
		delay_divisor =
		    delay_divisor_est40(delay_calibration_weight(25));
	}
	for (; delay_divisor > 0; delay_divisor--) {
		bus_space_write_4(bt, bh, PCC2REG_TIMER1_COUNTER, 0);
		bus_space_write_1(bt, bh, PCC2REG_TIMER1_CONTROL,
		    PCCTWO_TT_CTRL_CEN);
		delay(10000);
		bus_space_write_1(bt, bh, PCC2REG_TIMER1_CONTROL, 0);
		if (bus_space_read_4(bt, bh, PCC2REG_TIMER1_COUNTER) > 10000)
			break;	/* got it! */
	}

	bus_space_unmap(bt, bh, PCC2REG_SIZE);

	/* calculate cpuspeed */
	cpuspeed_khz = get_cpuspeed();
	if (cpuspeed_khz < 12500 || cpuspeed_khz > 60000) {
		printf("%s: Warning! Firmware has " \
		    "bogus CPU speed: `%s'\n", __func__, boardid.speed);
		cpuspeed_khz = ((cputype == CPU_68060)
		    ? delay_divisor_est60(delay_divisor)
		    : delay_divisor_est40(delay_divisor)) * 1000;
		printf("%s: Approximating speed using delay_divisor\n",
		    __func__);
	}
}

/*
 * Parse the `speed' field of Bug's boardid structure.  Returns
 * value in kHz.
 */
int
get_cpuspeed(void)
{
	int rv, i;

	for (i = 0, rv = 0; i < sizeof(boardid.speed); i++) {
		if (boardid.speed[i] < '0' || boardid.speed[i] > '9')
			return 0;
		rv = (rv * 10) + (boardid.speed[i] - '0');
	}

	return rv * 10;
}
#endif

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	{
		extern char end[];
		extern int *esym;

		ksyms_addsyms_elf((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		    (void *)&end, esym);
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * We want to print additional detail about on-board vs. VME memory.
 */
void
cpu_startup_print_total_memory(void (*pr)(const char *, ...)
			       __printflike(1, 2))
{
	u_quad_t vmememsize;
	char pbuf[9];
	u_int i;

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	(*pr)("total memory = %s", pbuf);

	for (vmememsize = 0, i = 1; i < VM_PHYSSEG_MAX; i++) {
		vmememsize +=
		    phys_seg_list[i].ps_end - phys_seg_list[i].ps_start;
	}
	if (vmememsize != 0) {
		format_bytes(pbuf, sizeof(pbuf),
		    phys_seg_list[0].ps_end - phys_seg_list[0].ps_start);
		(*pr)(" (%s on-board", pbuf);
		format_bytes(pbuf, sizeof(pbuf), vmememsize);
		(*pr)(", %s VMEbus)", pbuf);
	}

	(*pr)("\n");
}

void
machine_set_model(void)
{
	char board_str[16];
	char *suffix = (char *)&boardid.suffix;
	int len = snprintf(board_str, sizeof(board_str), "%x", machineid);

	if (suffix[0] != '\0' && len > 0 && len + 3 < sizeof(board_str)) {
		board_str[len++] = suffix[0];
		if (suffix[1] != '\0')
			board_str[len++] = suffix[1];
		board_str[len] = '\0';
	}
	cpu_setmodel("Motorola MVME-%s", board_str);
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

/* See: sig_machdep.c */

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	/* Save the RB_SBOOT flag. */
	howto |= (boothowto & RB_SBOOT);

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
	if (howto & RB_HALT) {
		printf("halted\n\n");
		doboot(RB_HALT);
		/* NOTREACHED */
	}

	printf("rebooting...\n");
	delay(1000000);
	doboot(RB_AUTOBOOT);
	/*NOTREACHED*/
}

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
int
nmihand(void *arg)
{

	mvme68k_abort("ABORT SWITCH");

	return 1;
}

/*
 * Common code for handling ABORT signals from buttons, switches,
 * serial lines, etc.
 */
void
mvme68k_abort(const char *cp)
{

#ifdef DDB
	db_printf("%s\n", cp);
	Debugger();
#else
#ifdef PANICBUTTON
	panic(cp);
#else
	printf("%s ignored\n", cp);
#endif /* PANICBUTTON */
#endif /* DDB */
}
