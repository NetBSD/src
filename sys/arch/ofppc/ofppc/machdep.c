/*	$NetBSD: machdep.c,v 1.92.14.3 2007/11/18 19:34:37 bouyer Exp $	*/
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.92.14.3 2007/11/18 19:34:37 bouyer Exp $");

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

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

struct pmap ofw_pmap;
char bootpath[256];
extern int console_node;

void ofwppc_batinit(void);
void	ofppc_bootstrap_console(void);

void
initppc(u_int startkernel, u_int endkernel, char *args)
{
	ofwoea_initppc(startkernel, endkernel, args);
}

void
cpu_startup(void)
{
	oea_startup(NULL);
}


void
consinit(void)
{
	ofwoea_consinit();
}


void
dumpsys(void)
{
	printf("dumpsys: TBD\n");
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void rtas_reboot(void);

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

	rtas_reboot();

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
 */

#define divrnd(n, q)	(((n)*2/(q)+1)/2)

void
ofppc_init_comcons(void)
{
#if (NCOM > 0)
	char name[64];
	uint32_t reg[2], comfreq;
	uint8_t dll, dlm;
	int speed, rate, err;
	bus_space_tag_t tag = &genppc_isa_io_space_tag;

	/* if we have a serial cons, we have work to do */
	memset(name, 0, sizeof(name));
	OF_getprop(console_node, "device_type", name, sizeof(name));
	if (strcmp(name, "serial") != 0)
		return;

	if (OF_getprop(console_node, "reg", reg, sizeof(reg)) == -1)
		return;

	if (OF_getprop(console_node, "clock-frequency", &comfreq, 4) == -1)
		comfreq = 0;

	if (comfreq == 0)
		comfreq = COM_FREQ;

	isa_outb(reg[1] + com_cfcr, LCR_DLAB);
	dll = isa_inb(reg[1] + com_dlbl);
	dlm = isa_inb(reg[1] + com_dlbh);
	rate = dll | (dlm << 8);
	isa_outb(reg[1] + com_cfcr, LCR_8BITS);
	speed = divrnd((comfreq / 16), rate);
	err = speed - (speed + 150)/300 * 300;
	speed -= err;
	if (err < 0)
		err = -err;
	if (err > 50)
		speed = 9600;

	/* Now we can attach the comcons */
	aprint_verbose("Switching to COM console at speed %d", speed);
	if (comcnattach(tag, reg[1], speed, comfreq, COM_TYPE_NORMAL,
	    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
		panic("Can't init serial console");
	aprint_verbose("\n");
#endif /*NCOM*/
}
