/*	$NetBSD: autoconf.c,v 1.19.20.1 2008/01/09 01:47:34 matt Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 * 	The Regents of the University of California. All rights reserved.
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
 * from: Utah $Hdr: autoconf.c 1.36 92/12/20$
 * 
 *	@(#)autoconf.c  8.2 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: autoconf.c 1.36 92/12/20$
 * 
 *	@(#)autoconf.c  8.2 (Berkeley) 1/12/94
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.19.20.1 2008/01/09 01:47:34 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/vmparam.h>
#include <machine/autoconf.h>
#include <machine/disklabel.h>
#include <machine/cpu.h>
#include <machine/pte.h>

#include <next68k/next68k/isr.h>
#include <next68k/next68k/nextrom.h>

#include <next68k/dev/intiovar.h>

volatile u_long *intrstat;
volatile u_long *intrmask;

static struct device *getdevunit(const char *, int);
static int devidentparse(const char *, int *, int *, int *);
static int atoi(const char *);

struct device_equiv {
	const char *alias;
	const char *real;
};
static struct device_equiv device_equiv[] = {
	{ "en", "xe" },
	{ "tp", "xe" },
};
static int ndevice_equivs = (sizeof(device_equiv)/sizeof(device_equiv[0]));

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
cpu_configure(void)
{
/* 	int dma_rev; */
	extern u_int rom_intrmask;
	extern u_int rom_intrstat;

	booted_device = NULL;	/* set by device drivers (if found) */

#if 0
	dma_rev = ((volatile u_char *)IIOV(NEXT_P_SCR1))[1];
	switch (dma_rev) {
	case 0:
		intrmask = (volatile u_long *)IIOV(NEXT_P_INTRMASK_0);
		intrstat = (volatile u_long *)IIOV(NEXT_P_INTRSTAT_0);
		/* dspreg = (volatile u_long *)IIOV(0x2007000); */
		break;
	case 1:
		intrmask = (volatile u_long *)IIOV(NEXT_P_INTRMASK);
		intrstat = (volatile u_long *)IIOV(NEXT_P_INTRSTAT);
		/* dspreg = (volatile u_long *)IIOV(0x2108000); */
		break;
	default:
		panic("unknown DMA chip revision");
	}
#else
	intrmask = (volatile u_long *)IIOV(rom_intrmask);
	intrstat = (volatile u_long *)IIOV(rom_intrstat);
	printf ("intrmask: %p\n", intrmask);
	printf ("intrstat: %p\n", intrstat);
#endif

	INTR_SETMASK(0);

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("autoconfig failed, no root");

	/* Turn on interrupts */
	spl0();
}

void
cpu_rootconf(void)
{
	int count, lun, part;
	
	count = lun = part = 0;

	devidentparse (rom_boot_info, &count, &lun, &part);
	booted_device = getdevunit (rom_boot_dev, count);
	
	printf("boot device: %s\n",
		(booted_device) ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, part);
}

/*
 * find a device matching "name" and unit number
 */
static struct device *
getdevunit(const char *name, int unit)
{
	struct device *dev = TAILQ_FIRST(&alldevs);
	char num[10], fullname[16];
	int lunit;
	int i;

	for (i = 0; i < ndevice_equivs; i++)
		if (device_equiv->alias && strcmp (name, device_equiv->alias) == 0)
			name = device_equiv->real;

	/* compute length of name and decimal expansion of unit number */
	sprintf(num, "%d", unit);
	lunit = strlen(num);
	if (strlen(name) + lunit >= sizeof(fullname) - 1)
		panic("config_attach: device name too long");

	strcpy(fullname, name);
	strcat(fullname, num);

	while (strcmp(dev->dv_xname, fullname) != 0) {
		if ((dev = TAILQ_NEXT(dev, dv_list)) == NULL)
			return NULL;
	}
	return dev;
}

/*
 * Parse a device ident.
 *
 * Format:
 *   (count, lun, part)
 */
static int
devidentparse(const char *spec, int *count, int *lun, int *part)
{
	int i;
	const char *args[3];

	if (*spec == '(') {
		/* tokenize device ident */
		args[0] = ++spec;
		for (i = 1; *spec && *spec != ')' && i<3; spec++) {
			if (*spec == ',')
				args[i++] = ++spec;
		}
		if (*spec != ')')
			goto baddev;
	
		switch(i) {
		case 3:
			*count  = atoi(args[0]);
			*lun  = atoi(args[1]);
			*part  = atoi(args[2]);
			break;
		case 2:
			*lun  = atoi(args[0]);
			*part  = atoi(args[1]);
			break;
		case 1:
			*part  = atoi(args[0]);
			break;
		case 0:
			break;
		}
	}
	else
		goto baddev;
    
	return 0;
    
 baddev:
	return ENXIO;
}

static int
atoi(const char *s)
{
	int val = 0;
	
	while(isdigit(*s))
		val = val * 10 + (*s++ - '0');
	return val;
}
