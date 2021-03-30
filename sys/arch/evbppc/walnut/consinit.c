/*	$NetBSD: consinit.c,v 1.10 2021/03/30 05:18:37 rin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.10 2021/03/30 05:18:37 rin Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <powerpc/ibm4xx/ibm405gp.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <powerpc/ibm4xx/dev/comopbvar.h>
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

void
consinit(void)
{

	com_opb_cnattach(COM_FREQ * 6, CONADDR, CONSPEED, comcnmode);
}

#ifdef KGDB
void
kgdb_port_init(void)
{
#if (NCOM > 0)
	if(!strcmp(kgdb_devname, "com")) {
		bus_space_tag_t tag = opb_get_bus_space_tag();
		com_kgdb_attach(tag, comkgdbaddr, comkgdbrate, COM_FREQ * 6,
		    COM_TYPE_NORMAL, comkgdbmode);
	}
#endif
}
#endif
