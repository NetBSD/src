/*	$NetBSD: machdep.c,v 1.81 2003/01/22 21:55:16 kleink Exp $	*/

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

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/boot_flag.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/db_machdep.h>
#include <ddb/db_extern.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <machine/bat.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>

#include <machine/platform.h>

#include <dev/cons.h>

/*
 * Global variables used here and there
 */
char bootpath[256];

int lcsplx(int);			/* called from locore.S */

static int fake_spl __P((int));
static void fake_splx __P((int));
static void fake_setsoft __P((int));
static void fake_clock_return __P((struct clockframe *, int));
static void *fake_intr_establish __P((int, int, int, int (*)(void *), void *));
static void fake_intr_disestablish __P((void *));

struct machvec machine_interface = {
	fake_spl,
	fake_spl,
	fake_splx,
	fake_setsoft,
	fake_clock_return,
	fake_intr_establish,
	fake_intr_disestablish,
};

void	ofppc_bootstrap_console(void);

struct pmap ofw_pmap;

void
initppc(startkernel, endkernel, args)
	u_int startkernel, endkernel;
	char *args;
{
#ifdef DDB
	extern void *startsym, *endsym;
#endif

	/* Initialize the bootstrap console. */
	ofppc_bootstrap_console();

	/*
	 * Initialize the bat registers
	 */
	mpc6xx_batinit(0);

	/*
	 * Initialize the platform structure.  This may add entries
	 * to the BAT table.
	 */
	platform_init();

#ifdef __notyet__	/* Needs some rethinking regarding real/virtual OFW */
	OF_set_callback(callback);
#endif

	mpc6xx_init(NULL);

	/*
	 * Now that translation is enabled (and we can access bus space),
	 * initialize the console.
	 */
	(*platform.cons_init)();

	/*
	 * Parse arg string.
	 */
	strcpy(bootpath, args);
	while (*++args && *args != ' ');
	if (*args) {
		for(*args++ = 0; *args; args++)
			BOOT_FLAG(*args, boothowto);
	}

	/*
	 * Set the page size.
	 */
	uvm_setpagesize();

	/*
	 * Initialize pmap module.
	 */
	pmap_bootstrap(startkernel, endkernel);

#ifdef DDB
	ddb_init((int)((u_int)endsym - (u_int)startsym), startsym, endsym);
	if (boothowto & RB_KDB)
		Debugger();
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

/*
 * Machine dependent startup code.
 */
void
cpu_startup()
{

	mpc6xx_startup(NULL);

	/*
	 * Now allow hardware interrupts.
	 */
	splhigh();
	mtmsr(mfmsr() | PSL_EE | PSL_RI);
	(*platform.softintr_init)();
}

void
consinit()
{

	(*cn_tab->cn_probe)(cn_tab);
}

void	ofcons_cnprobe(struct consdev *);
int	ofppc_cngetc(dev_t);
void	ofppc_cnputc(dev_t, int);

struct consdev ofppc_bootcons = {
	ofcons_cnprobe, NULL, ofppc_cngetc, ofppc_cnputc, nullcnpollc, NULL,
	    makedev(0,0), 1,
};

int	ofppc_stdin_ihandle, ofppc_stdout_ihandle;
int	ofppc_stdin_phandle, ofppc_stdout_phandle;

void
ofppc_bootstrap_console(void)
{
	int chosen;
	char data[4];

	chosen = OF_finddevice("/chosen");

	if (OF_getprop(chosen, "stdin", data, sizeof(data)) != sizeof(int))
		goto nocons;
	ofppc_stdin_ihandle = of_decode_int(data);
	ofppc_stdin_phandle = OF_instance_to_package(ofppc_stdin_ihandle);

	if (OF_getprop(chosen, "stdout", data, sizeof(data)) != sizeof(int))
		goto nocons;
	ofppc_stdout_ihandle = of_decode_int(data);
	ofppc_stdout_phandle = OF_instance_to_package(ofppc_stdout_ihandle);

	cn_tab = &ofppc_bootcons;

 nocons:
	return;
}

int
ofppc_cngetc(dev_t dev)
{
	u_char ch = '\0';
	int l;

	while ((l = OF_read(ofppc_stdin_ihandle, &ch, 1)) != 1)
		if (l != -2 && l != 0)
			return (-1);

	return (ch);
}

void
ofppc_cnputc(dev_t dev, int c)
{
	char ch = c;

	OF_write(ofppc_stdout_ihandle, &ch, 1);
}

/*
 * Crash dump handling.
 */

/*
 * Stray interrupts.
 */
void
strayintr(irq)
	int irq;
{
	log(LOG_ERR, "stray interrupt %d\n", irq);
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(howto, what)
	int howto;
	char *what;
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		printf("halted\n\n");
		ppc_exit();
	}
	if (!cold && (howto & RB_DUMP))
		mpc6xx_dumpsys();
	doshutdownhooks();
	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;
	ppc_boot(str);
}

#ifdef notyet
/*
 * OpenFirmware callback routine
 */
void
callback(p)
	void *p;
{
	panic("callback");	/* for now			XXX */
}
#endif

/*
 * Perform an `splx()' for locore.
 */
int
lcsplx(int ipl)
{

	return (_spllower(ipl));
}

/*
 * Initial Machine Interface.
 */
static int
fake_spl(int new)
{
	int scratch;

	asm volatile ("mfmsr %0; andi. %0,%0,%1; mtmsr %0; isync"
	    : "=r"(scratch) : "K"((u_short)~(PSL_EE|PSL_ME)));
	return (-1);
}

static void
fake_setsoft(int ipl)
{
	/* Do nothing */
}

static void
fake_splx(new)
	int new;
{

	(void) fake_spl(0);
}

static void
fake_clock_return(frame, nticks)
	struct clockframe *frame;
	int nticks;
{
	/* Do nothing */
}

static void *
fake_intr_establish(irq, level, ist, handler, arg)
	int irq, level, ist;
	int (*handler) __P((void *));
	void *arg;
{

	panic("fake_intr_establish");
}

static void
fake_intr_disestablish(cookie)
	void *cookie;
{

	panic("fake_intr_disestablish");
}
