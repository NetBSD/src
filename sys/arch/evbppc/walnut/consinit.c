/*	$NetBSD: consinit.c,v 1.3 2003/07/15 01:37:37 lukem Exp $	*/

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.3 2003/07/15 01:37:37 lukem Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/bus.h>

#include <powerpc/ibm4xx/ibm405gp.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include <dev/cons.h>

#ifndef CONSDEVNAME
#define CONSDEVNAME "com"
#endif

#if (NCOM > 0)
#ifndef CONADDR
#define CONADDR IBM405GP_UART0_BASE
#endif
#ifndef CONSPEED
#define CONSPEED B9600	/*  */
// #define CONSPEED B115200	/* 9600 is too slow for my taste */
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comcnmode = CONMODE;
#endif /* NCOM */

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME "com"
#endif
char kgdb_devname[] = KGDB_DEVNAME;

#if (NCOM > 1)
#ifndef KGDB_DEVADDR
#define KGDB_DEVADDR  UART1_BASE
#endif
int comkgdbaddr = KGDB_DEVADDR;

#ifndef KGDB_DEVRATE
#define KGDB_DEVRATE CONSPEED
#endif
int comkgdbrate = KGDB_DEVRATE;

#ifndef KGDB_DEVMODE
#define KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comkgdbmode = KGDB_DEVMODE;

#endif /* NCOM */

#endif /* KGDB */

/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from initppc either.
 */
void
consinit(void)
{
	static int initted = 0;
#if (NCOM > 0)
	bus_space_tag_t tag;
#endif

	if (initted)
		return;
	initted = 1;

#if (NCOM > 0)
	tag = ibm4xx_make_bus_space_tag(0, 0);

	if (comcnattach(tag, CONADDR, CONSPEED, COM_FREQ*6,
	    COM_TYPE_NORMAL, comcnmode))
		panic("can't init serial console @%x", CONADDR);
	else
		return;
#endif
	panic("console device missing -- serial console not in kernel");
	/* Of course, this is moot if there is no console... */
}

#ifdef KGDB
void
kgdb_port_init(void)
{
#if (NCOM > 0)
	if(!strcmp(kgdb_devname, "com")) {
		bus_space_tag_t tag = ibm4xx_make_bus_space_tag(0, 2);
		com_kgdb_attach(tag, comkgdbaddr, comkgdbrate, COM_FREQ * 6,
		    COM_TYPE_NORMAL, comkgdbmode);
	}
#endif
}
#endif
