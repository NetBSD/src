/*	$NetBSD: consinit.c,v 1.26.2.1 2014/08/20 00:03:25 tls Exp $	*/

/*-
 * Copyright (c) 1999 Eduardo E. Horvath
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.26.2.1 2014/08/20 00:03:25 tls Exp $");

#include "opt_ddb.h"
#include "pcons.h"
#include "ukbd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/bsd_openprom.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/psl.h>
#include <machine/z8530var.h>
#include <machine/sparc64.h>

#include <dev/cons.h>

#include <sparc64/dev/cons.h>

#include <dev/usb/ukbdvar.h>

static void prom_cnprobe(struct consdev *);
static void prom_cninit(struct consdev *);
int  prom_cngetc(dev_t);
static void prom_cnputc(dev_t, int);
static void prom_cnpollc(dev_t, int);

/*
 * The console is set to this one initially,
 * which lets us use the PROM until consinit()
 * is called to select a real console.
 */
struct consdev consdev_prom = {
	.cn_probe = prom_cnprobe,
	.cn_init = prom_cninit,
	.cn_getc = prom_cngetc,
	.cn_putc = prom_cnputc,
	.cn_pollc = prom_cnpollc,
};

void
prom_cnprobe(struct consdev *cd)
{
#if NPCONS > 0
	int maj;
	extern const struct cdevsw pcons_cdevsw;

	maj = cdevsw_lookup_major(&pcons_cdevsw);
	cd->cn_dev = makedev(maj, 0);
	cd->cn_pri = CN_INTERNAL;
#endif
}

int
prom_cngetc(dev_t dev)
{
	unsigned char ch = '\0';
	int l;
#ifdef DDB
	static int nplus = 0;
#endif

	while ((l = prom_read(prom_stdin(), &ch, 1)) != 1)
		/* void */;
#ifdef DDB
	if (ch == '+') {
		if (nplus++ > 3) Debugger();
	} else nplus = 0;
#endif
	if (ch == '\r')
		ch = '\n';
	return ch;
}

static void
prom_cninit(struct consdev *cn)
{
}

/*
 * PROM console output putchar.
 */
static void
prom_cnputc(dev_t dev, int c)
{
	int s;
	char c0 = (c & 0x7f);

	s = splhigh();
	prom_write(prom_stdout(), &c0, 1);
	splx(s);
}

void
prom_cnpollc(dev_t dev, int on)
{
	if (on) {
                /* Entering debugger. */
#if NFB > 0
                fb_unblank();
#endif
	} else {
                /* Resuming kernel. */
	}
#if NPCONS > 0
	pcons_cnpollc(dev, on);
#endif
}

/*****************************************************************/

#ifdef	DEBUG
#define	DBPRINT(x)	prom_printf x
#else
#define	DBPRINT(x)
#endif

int prom_stdin_node;
int prom_stdout_node;

/*
 * This function replaces sys/dev/cninit.c
 * Determine which device is the console using
 * the PROM "input source" and "output sink".
 */
void
consinit(void)
{
	char buffer[128];
	const char *consname = "unknown";

	DBPRINT(("consinit()\r\n"));

	if (cn_tab != &consdev_prom)
		return;

	if ((prom_stdin_node = prom_instance_to_package(prom_stdin())) == 0) {
		printf("WARNING: no PROM stdin\n");
	}
	DBPRINT(("stdin node = %x\r\n", prom_stdin_node));

	if ((prom_stdout_node = prom_instance_to_package(prom_stdout())) == 0)
		printf("WARNING: no PROM stdout\n");
	DBPRINT(("stdout package = %x\r\n", prom_stdout_node));

	DBPRINT(("buffer @ %p\r\n", buffer));

	if (prom_stdin_node != 0 &&
	    (prom_getproplen(prom_stdin_node, "keyboard") >= 0)) {
#if NUKBD > 0
		if ((OF_instance_to_path(prom_stdin(), buffer, sizeof(buffer)) >= 0) &&
		    (strstr(buffer, "/usb@") != NULL)) {
			/*
		 	* If we have a USB keyboard, it will show up as (e.g.)
		 	*   /pci@1f,0/usb@c,3/keyboard@1	(Blade 100)
		 	*/
			consname = "usb-keyboard/display";
			ukbd_cnattach();
		} else
#endif
			consname = "sun-keyboard/display";
	} else if (prom_stdin_node != 0 &&
		   (OF_instance_to_path(prom_stdin(), buffer, sizeof(buffer)) >= 0)) {
		consname = buffer;
	}
	DBPRINT(("console is %s\n", consname));
#ifndef DEBUG
	(void)consname;
#endif

	/* Initialize PROM console */
	(*cn_tab->cn_probe)(cn_tab);
	(*cn_tab->cn_init)(cn_tab);
}
