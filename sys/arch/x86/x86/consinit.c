/*	$NetBSD: consinit.c,v 1.15.32.2 2009/11/01 13:58:17 jym Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.15.32.2 2009/11/01 13:58:17 jym Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/bootinfo.h>
#include <arch/x86/include/genfb_machdep.h>

#include "genfb.h"
#include "vga.h"
#include "ega.h"
#include "pcdisplay.h"
#if (NVGA > 0) || (NEGA > 0) || (NPCDISPLAY > 0)
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#if (NVGA > 0)
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif
#if (NEGA > 0)
#include <dev/isa/egavar.h>
#endif
#if (NPCDISPLAY > 0)
#include <dev/isa/pcdisplayvar.h>
#endif
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#include <dev/pckbport/pckbportvar.h>
#endif
#include "pckbd.h" /* for pckbc_machdep_cnattach */

#if (NGENFB > 0)
#include <dev/wsfb/genfbvar.h>
#endif

#ifdef __i386__
#include "xboxfb.h"
#if (NXBOXFB > 0)
#include <machine/xbox.h>
#endif
#endif

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include "ukbd.h"
#if (NUKBD > 0)
#include <dev/usb/ukbdvar.h>
#endif

#ifndef CONSDEVNAME
#define CONSDEVNAME "pc"
#endif

#if (NCOM > 0)
#ifndef CONADDR
#define CONADDR 0x3f8
#endif
#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comcnmode = CONMODE;
#endif /* NCOM */

const struct btinfo_console default_consinfo = {
	{0, 0},
	CONSDEVNAME,
#if (NCOM > 0)
	CONADDR, CONSPEED
#else
	0, 0
#endif
};

#ifdef KGDB
#ifndef KGDB_DEVNAME
#define KGDB_DEVNAME "com"
#endif
const char kgdb_devname[] = KGDB_DEVNAME;

#if (NCOM > 0)
#ifndef KGDB_DEVADDR
#define KGDB_DEVADDR 0x3f8
#endif
int comkgdbaddr = KGDB_DEVADDR;
#ifndef KGDB_DEVRATE
#define KGDB_DEVRATE TTYDEF_SPEED
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
 * it shouldn't be called from init386 either.
 */
void
consinit(void)
{
	const struct btinfo_console *consinfo;
	const struct btinfo_framebuffer *fbinfo;
	static int initted;

	if (initted)
		return;
	initted = 1;

#ifndef CONS_OVERRIDE
	consinfo = lookup_bootinfo(BTINFO_CONSOLE);
	if (!consinfo)
#endif
		consinfo = &default_consinfo;

	fbinfo = lookup_bootinfo(BTINFO_FRAMEBUFFER);

	if (!strcmp(consinfo->devname, "pc")) {
		int error;
#if (NGENFB > 0)
		if (fbinfo && fbinfo->physaddr > 0) {
			if (x86_genfb_cnattach() == -1) {
				initted = 0;	/* defer */
				return;
			}
			genfb_cnattach();
			goto dokbd;
		}
		genfb_disable();
#endif
#if (NXBOXFB > 0)
		switch (xboxfb_cnattach()) {
		case 0:
			goto dokbd;
		case 1:
			break;
		case -1:
			/* defer initialization until later */
			initted = 0;
			return;
		}
#endif
#if (NVGA > 0)
		if (!vga_cnattach(X86_BUS_SPACE_IO, X86_BUS_SPACE_MEM,
				  -1, 1))
			goto dokbd;
#endif
#if (NEGA > 0)
		if (!ega_cnattach(X86_BUS_SPACE_IO, X86_BUS_SPACE_MEM))
			goto dokbd;
#endif
#if (NPCDISPLAY > 0)
		if (!pcdisplay_cnattach(X86_BUS_SPACE_IO, X86_BUS_SPACE_MEM))
			goto dokbd;
#endif
		if (0) goto dokbd; /* XXX stupid gcc */
dokbd:
		error = ENODEV;
#if (NPCKBC > 0)
		error = pckbc_cnattach(X86_BUS_SPACE_IO, IO_KBD, KBCMDP,
		    PCKBC_KBD_SLOT);
#endif
#if (NUKBD > 0)
		if (error)
			error = ukbd_cnattach();
#endif
		if (error)
			printf("WARNING: no console keyboard, error=%d\n",
			       error);
		return;
	}
#if (NCOM > 0)
	if (!strcmp(consinfo->devname, "com")) {
		bus_space_tag_t tag = X86_BUS_SPACE_IO;
		int addr = consinfo->addr;
		int speed = consinfo->speed;

		if (addr == 0)
			addr = CONADDR;
		if (speed == 0)
			speed = CONSPEED;

		if (comcnattach(tag, addr, speed,
				COM_FREQ, COM_TYPE_NORMAL, comcnmode))
			panic("can't init serial console @%x", consinfo->addr);

		return;
	}
#endif
	panic("invalid console device %s", consinfo->devname);
}

#ifdef KGDB
void
kgdb_port_init(void)
{
#if (NCOM > 0)
	if(!strcmp(kgdb_devname, "com")) {
		bus_space_tag_t tag = X86_BUS_SPACE_IO;

		com_kgdb_attach(tag, comkgdbaddr, comkgdbrate, COM_FREQ, 
		    COM_TYPE_NORMAL, comkgdbmode);
	}
#endif
}
#endif
