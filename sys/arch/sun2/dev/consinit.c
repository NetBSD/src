/*	$NetBSD: consinit.c,v 1.3 2003/07/15 03:36:11 lukem Exp $	*/

/*-
 * Copyright (c) 2001 Matthew Fredette
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
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.3 2003/07/15 03:36:11 lukem Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "pcons.h"
#include "kbd.h"
#include "zs.h"

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
#include <sys/kgdb.h>

#include <machine/autoconf.h>
#include <machine/promlib.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/psl.h>
#include <machine/z8530var.h>

#include <dev/cons.h>

#include <sun2/dev/cons.h>

static void prom_cnprobe __P((struct consdev *));
static void prom_cninit __P((struct consdev *));
int  prom_cngetc __P((dev_t));
static void prom_cnputc __P((dev_t, int));
static void prom_cnpollc __P((dev_t, int));
static void prom_cnputc __P((dev_t, int));

#ifdef	PROM_OBP_V2
/*    
 * The following several variables are related to
 * the configuration process, and are used in initializing 
 * the machine.
 */     
int	prom_stdin_node;	/* node ID of ROM's console input device */
int	prom_stdout_node;	/* node ID of ROM's console output device */
char	prom_stdin_args[16];
char	prom_stdout_args[16];
#endif	/* PROM_OBP_V2 */

/*
 * The console is set to this one initially,
 * which lets us use the PROM until consinit()
 * is called to select a real console.
 */
struct consdev consdev_prom = {
	prom_cnprobe,
	prom_cninit,
	prom_cngetc,
	prom_cnputc,
	prom_cnpollc,
	NULL,
};

/*
 * The console table pointer is statically initialized
 * to point to the PROM (output only) table, so that
 * early calls to printf will work.
 */
struct consdev *cn_tab = &consdev_prom;

void
prom_cnprobe(cd)
	struct consdev *cd;
{
#if NPCONS > 0
	extern const struct cdevsw pcons_cdevsw;

	cd->cn_dev = makedev(cdevsw_lookup_major(&pcons_cdevsw), 0);
	cd->cn_pri = CN_INTERNAL;
#endif
}

int
prom_cngetc(dev)
	dev_t dev;
{
	int ch;
#ifdef DDB
	static int nplus = 0;
#endif
	
	ch = prom_getchar();
#ifdef DDB
	if (ch == '+') {
		if (nplus++ > 3) Debugger();
	} else nplus = 0;
#endif
	return ch;
}

static void
prom_cninit(cn)
	struct consdev *cn;
{
}

/*
 * PROM console output putchar.
 */
static void
prom_cnputc(dev, c)
	dev_t dev;
	int c;
{
	int s;

	s = splhigh();
	prom_putchar(c);
	splx(s);
}

void
prom_cnpollc(dev, on)
	dev_t dev;
	int on;
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

#ifdef notyet /* PROM_OBP_V2 */
void
prom_get_device_args(prop, dev, dev_sz, args, args_sz)
	const char *prop;
	char *dev;
	unsigned int dev_sz;
	char *args;
	unsigned int args_sz;
{
	char *cp, buffer[128];
                
	getpropstringA(prom_findroot(), (char *)prop, buffer, sizeof buffer);
        
	/*
 	* Extract device-specific arguments from a PROM device path (if any)
 	*/
	cp = buffer + strlen(buffer);
	while (cp >= buffer) {
		if (*cp == ':') {
			strncpy(args, cp+1, args_sz);
			*cp = '\0';
			strncpy(dev, buffer, dev_sz);
			break;
		}
		cp--;
	}
}
#endif	/* PROM_OBP_V2 */

/*
 * This function replaces sys/dev/cninit.c
 * Determine which device is the console using
 * the PROM "input source" and "output sink".
 */
void
consinit()
{
#ifdef	notyet /* PROM_OBP_V2 */
	char buffer[128];
#endif	/* PROM_OBP_V2 */
	char *consname = "unknown";
#if KGDB
#if NZS > 0
	extern const struct cdevsw zstty_cdevsw;
#endif
#endif
	
	DBPRINT(("consinit()\r\n"));
	if (cn_tab != &consdev_prom) return;

	switch(prom_version()) {
#ifdef	PROM_OLDMON
	case PROM_OLDMON:
	case PROM_OBP_V0:
		switch(prom_stdin()) {
		case PROMDEV_KBD:
			consname = "keyboard/display";
			break;
		case PROMDEV_TTYA:
			consname = "ttya";
			break;
		case PROMDEV_TTYB:
			consname = "ttyb";
			break;
		}
		break;
#endif	/* PROM_OLDMON */
	
#ifdef	notyet /* PROM_OBP_V2 */
	case PROM_OBP_V2:
	case PROM_OBP_V3:
	case PROM_OPENFIRM:
	
		/* Save PROM arguments for device matching */
		prom_get_device_args("stdin-path", 
				     buffer, 
				     sizeof(buffer),
				     prom_stdin_args,
				     sizeof(prom_stdin_args));
		prom_get_device_args("stdout-path",
				     buffer,
				     sizeof(buffer),
				     prom_stdout_args,
				     sizeof(prom_stdout_args));

		/*
		 * Translate the STDIO package instance (`ihandle') -- that
		 * the PROM has already opened for us -- to a device tree
		 * node (i.e. a `phandle'). 
		 */
		DBPRINT(("stdin instance = %x\r\n", prom_stdin()));
		if ((prom_stdin_node = prom_instance_to_package(prom_stdin())) == 0) {
			printf("WARNING: no PROM stdin\n");
		} 
		DBPRINT(("stdin package = %x\r\n", prom_stdin_node));
		
		DBPRINT(("stdout instance = %x\r\n", prom_stdout()));
		if ((prom_stdout_node = prom_instance_to_package(prom_stdout())) == 0) {
			printf("WARNING: no PROM stdout\n");
		}
		DBPRINT(("stdout package = %x\r\n", prom_stdout_node));
		DBPRINT(("buffer @ %p\r\n", buffer));
	
		if (prom_stdin_node && prom_node_has_property(prom_stdin_node, "keyboard") {
#if NKBD == 0		
			printf("cninit: kdb/display not configured\n");
#endif
			consname = "keyboard/display";
		} else if (prom_stdout_node)
			consname = buffer;
#endif	/* PROM_OBP_V2 */
	}
	printf("console is %s\n", consname);
 
	/* Initialize PROM console */
	(*cn_tab->cn_probe)(cn_tab);
	(*cn_tab->cn_init)(cn_tab);

#ifdef	KGDB
	/* Set up KGDB */
#if NZS > 0
	if (cdevsw_lookup(kgdb_dev) == &zstty_cdevsw)
		zs_kgdb_init();
#endif	/* NZS > 0 */
#endif	/* KGDB */
}

