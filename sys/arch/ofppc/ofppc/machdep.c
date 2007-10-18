/*	$NetBSD: machdep.c,v 1.92.12.1 2007/10/18 08:32:24 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.92.12.1 2007/10/18 08:32:24 yamt Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/mount.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>
#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/bus.h>
#include <machine/isa_machdep.h>

#include <powerpc/oea/bat.h>
#include <powerpc/ofw_cons.h>


struct pmap ofw_pmap;
char bootpath[256];

void ofwppc_batinit(void);
void	ofppc_bootstrap_console(void);

void
initppc(u_int startkernel, u_int endkernel, char *args)
{
	/* Initialize the bootstrap console. */
	ofppc_bootstrap_console();
	printf( "\n\nHallo Welt!\n\n");
	ofwoea_initppc(startkernel, endkernel, args);
	map_isa_ioregs();
}

void
cpu_startup(void)
{
	oea_startup(NULL);
}

/*
void
consinit(void)
{
	ofwoea_consinit();
}
*/

void
dumpsys(void)
{
	printf("dumpsys: TBD\n");
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */

void
cpu_reboot(int howto, char *what)
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();         /* sync */
		resettodr();            /* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		printf("halted\n\n");
		ppc_exit();
	}
	if (!cold && (howto & RB_DUMP))
		oea_dumpsys();
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

/*
 * XXX
 * The following code is subject to die at a later date.  This is the only
 * remaining code in this file subject to the Tools GmbH copyright.
 */

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
	    NULL, NULL, makedev(0,0), 1,
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

