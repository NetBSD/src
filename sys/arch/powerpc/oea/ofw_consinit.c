/* $NetBSD: ofw_consinit.c,v 1.25 2022/02/13 12:24:24 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ofw_consinit.c,v 1.25 2022/02/13 12:24:24 martin Exp $");

#include "adb.h"
#include "adbkbd.h"
#include "akbd.h"
#include "isa.h"
#include "ofb.h"
#include "pckbc.h"
#include "ukbd.h"
#include "wsdisplay.h"
#include "zsc.h"
#include "zstty.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/tty.h>

#include <prop/proplib.h>

#include <machine/autoconf.h>
#include <machine/trap.h>
#include <sys/bus.h>

#include <powerpc/ofw_cons.h>
#include <powerpc/ofw_machdep.h>

#include <dev/cons.h>
#include <dev/ofw/openfirm.h>

#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wscons_callbacks.h>

#if NZSC > 0
#include <machine/z8530var.h>
#endif

#if (NADB > 0)
#include <macppc/dev/adbvar.h>
#endif

#if (NUKBD > 0)
#include <dev/usb/ukbdvar.h>
struct usb_kbd_ihandles {
	struct usb_kbd_ihandles *next;
	int ihandle;
};
#endif

#if (NZSTTY > 0)
#include <dev/ic/z8530reg.h>
extern struct consdev consdev_zs;
#endif

#if (NPCKBC > 0)
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/pckbcvar.h>
#endif

extern int console_node, console_instance;

int ofkbd_ihandle = -1;

static void ofwoea_cnprobe_keyboard(void);

/*#define OFDEBUG*/

#ifdef OFDEBUG
void ofprint(const char *, ...);

void ofprint(const char *blah, ...)
{
	va_list va;
	char buf[256];
	int len;

	va_start(va, blah);
	len = vsnprintf(buf, sizeof(buf), blah, va);
	va_end(va);
	OF_write(console_instance, buf, len);
}

#define OFPRINTF ofprint
#else
#define OFPRINTF while(0) printf
#endif

bool ofwoea_use_serial_console;
static struct consdev *selected_serial_consdev;

static int (*selected_keyboard)(void);

/* XXX Gross. */
#if NPCKBC > 0
static int
ofwoea_pckbd_cnattach(void)
{
	return pckbc_cnattach(&genppc_isa_io_space_tag, IO_KBD, KBCMDP,
	    PCKBC_KBD_SLOT, 0);
}
#endif

void
ofwoea_cnprobe(void)
{
	char name[32];

	OFPRINTF("console node: %08x\n", console_node);

	if (console_node == -1)
		return;

	memset(name, 0, sizeof(name));
	if (OF_getprop(console_node, "device_type", name, sizeof(name)) == -1)
		return;

	OFPRINTF("console type: %s\n", name);

	if (strcmp(name, "serial") == 0) {
		ofwoea_use_serial_console = true;
#ifdef PMAC_G5
		/* The MMU hasn't been initialized yet, use failsafe for now */
		extern struct consdev failsafe_cons;
		selected_serial_consdev = &failsafe_cons;
		aprint_verbose("Early G5 console selected "
		    "(keeping OF console for now)\n");
		return;
#endif /* PMAC_G5 */

#if (NZSTTY > 0) && !defined(MAMBO)
		OF_getprop(console_node, "name", name, sizeof(name));
		if (strcmp(name, "ch-a") == 0 || strcmp(name, "ch-b") == 0) {
			selected_serial_consdev = &consdev_zs;
		}
		return;
#endif /* NZTTY */

		/* fallback to OFW boot console (already set) */
		return;
	}

	/*
	 * We're going to use a display console.  Probe for the keyboard
	 * we'll use.
	 */
	ofwoea_cnprobe_keyboard();
}

/*
 * XXX This routine is a complete disaster, filled with platform-specific
 * XXX stuff.  Fix, plz.
 */
static void
ofwoea_cnprobe_keyboard(void)
{
	extern int ofw_stdin;

	int node, kstdin = ofw_stdin;
	char name[16];
#if (NAKBD > 0) || (NADBKBD > 0)
	int akbd;
#endif
#if NUKBD > 0
	struct usb_kbd_ihandles *ukbds;
	int ukbd;
#endif

	/*
	 * We must determine which keyboard type we have.
	 */
	node = OF_instance_to_package(kstdin);
	memset(name, 0, sizeof(name));
	OF_getprop(node, "name", name, sizeof(name));
	if (strcmp(name, "keyboard") != 0) {
		printf("WARNING: stdin is not a keyboard: %s\n", name);
		return;
	}

	memset(name, 0, sizeof(name));
	OF_getprop(OF_parent(node), "name", name, sizeof(name));
#if NAKBD > 0
	if (strcmp(name, "adb") == 0) {
		printf("console keyboard type: ADB\n");
		selected_keyboard = akbd_cnattach;
		goto kbd_found;
	}
#endif
#if NADBKBD > 0
	if (strcmp(name, "adb") == 0) {
		printf("console keyboard type: ADB\n");
		selected_keyboard = adbkbd_cnattach;
		goto kbd_found;
	}
#endif
#if NPCKBC > 0
	if (strcmp(name, "isa") == 0) {
		printf("console keyboard type: PC Keyboard\n");
		selected_keyboard = ofwoea_pckbd_cnattach;
		goto kbd_found;
	}
#endif

	/*
	 * It is not obviously an ADB/PC keyboard. Could be USB,
	 * or ADB on some firmware versions (e.g.: iBook G4)
	 * This is not enough, we have a few more problems:
	 *
	 *	(1) The stupid Macintosh firmware uses a
	 *	    `psuedo-hid' (no typo) or `pseudo-hid',
	 *	    which apparently merges all keyboards
	 *	    input into a single input stream.
	 *	    Because of this, we can't actually
	 *	    determine which controller or keyboard
	 *	    is really the console keyboard!
	 *
	 *	(2) Even if we could, the keyboard can be USB,
	 *	    and this requires a lot of the kernel to
	 *	    be running in order for it to work.
	 *
	 *      (3) If the keyboard is behind isa, we don't have enough
	 * 	    kernel setup to use it yet, so punt to the ofroutines.
	 *
	 * So, what we do is this:
	 *
	 *	(1) First check for OpenFirmware implementation
	 *	    that will not let us distinguish between
	 *	    USB and ADB. In that situation, try attaching
	 *	    anything as we can, and hope things get better
	 *	    at autoconfiguration time.
	 *
	 *	(2) Assume the keyboard is USB.
	 *	    Tell the ukbd driver that it is the console.
	 *	    At autoconfiguration time, it will attach the
	 *	    first USB keyboard instance as the console
	 *	    keyboard.
	 *
	 *	(3) Until then, so that we have _something_, we
	 *	    use the OpenFirmware I/O facilities to read
	 *	    the keyboard.
	 */

	/*
	 * stdin is /pseudo-hid/keyboard.  There is no
	 * `adb-kbd-ihandle or `usb-kbd-ihandles methods
	 * available. Try attaching as ADB.
	 * But only if ADB support is actually present.
	 *
	 * XXX This must be called before pmap_bootstrap().
	 */
	if (strcmp(name, "pseudo-hid") == 0) {
		int adb_node;

		adb_node = OF_finddevice("/pci/mac-io/via-pmu/adb");
		if (adb_node > 0) {
			printf("ADB support found\n");
#if NAKBD > 0
			selected_keyboard = akbd_cnattach;
#endif
#if NADBKBD > 0
			selected_keyboard = adbkbd_cnattach;
#endif
		} else {
			/* must be USB */
			printf("No ADB support present, assuming USB "
			       "keyboard\n");
#if NUKBD > 0
			selected_keyboard = ukbd_cnattach;
#endif
		}
		goto kbd_found;
	}

	/*
	 * stdin is /psuedo-hid/keyboard.  Test `adb-kbd-ihandle and
	 * `usb-kbd-ihandles to figure out the real keyboard(s).
	 *
	 * XXX This must be called before pmap_bootstrap().
	 */

#if NUKBD > 0
	if (OF_call_method("`usb-kbd-ihandles", kstdin, 0, 1, &ukbds) >= 0 &&
	    ukbds != NULL && ukbds->ihandle != 0 &&
	    OF_instance_to_package(ukbds->ihandle) != -1) {
		printf("usb-kbd-ihandles matches\n");
		printf("console keyboard type: USB\n");
		selected_keyboard = ukbd_cnattach;
		goto kbd_found;
	}
	/* Try old method name. */
	if (OF_call_method("`usb-kbd-ihandle", kstdin, 0, 1, &ukbd) >= 0 &&
	    ukbd != 0 &&
	    OF_instance_to_package(ukbd) != -1) {
		printf("usb-kbd-ihandle matches\n");
		printf("console keyboard type: USB\n");
		kstdin = ukbd;
		selected_keyboard = ukbd_cnattach;
		goto kbd_found;
	}
#endif

#if (NAKBD > 0) || (NADBKBD > 0)
	if (OF_call_method("`adb-kbd-ihandle", kstdin, 0, 1, &akbd) >= 0 &&
	    akbd != 0 &&
	    OF_instance_to_package(akbd) != -1) {
		printf("adb-kbd-ihandle matches\n");
		printf("console keyboard type: ADB\n");
		kstdin = akbd;
#if NAKBD > 0
		selected_keyboard = akbd_cnattach;
#endif
#if NADBKBD > 0
		selected_keyboard = adbkbd_cnattach;
#endif
		goto kbd_found;
	}
#endif

#if NUKBD > 0
	/*
	 * XXX Old firmware does not have `usb-kbd-ihandles method.  Assume
	 * XXX USB keyboard anyway.
	 */
	printf("defaulting to USB...");
	printf("console keyboard type: USB\n");
	selected_keyboard = ukbd_cnattach;
	goto kbd_found;
#endif

	/*
	 * No keyboard is found.  Just return.
	 */
	printf("no console keyboard\n");
	return;

kbd_found:
	ofkbd_ihandle = kstdin;
}

/*
 * Bootstrap console keyboard routines, using OpenFirmware I/O.
 */
int
ofkbd_cngetc(dev_t dev)
{
	u_char c = '\0';
	int len;

	KASSERT(ofkbd_ihandle != -1);

	do {
		len = OF_read(ofkbd_ihandle, &c, 1);
	} while (len != 1);

	return c;
}

void
cninit(void)
{
	if (ofwoea_use_serial_console) {
		if (selected_serial_consdev != NULL) {
			cn_tab = selected_serial_consdev;
			(*cn_tab->cn_probe)(cn_tab);
			(*cn_tab->cn_init)(cn_tab);
		}
		return;
	}

#if NWSDISPLAY > 0
	rascons_cnattach();
#endif
	if (selected_keyboard != NULL) {
		(*selected_keyboard)();

#if NWSDISPLAY > 0
		/*
		 * XXX This is a little gross, but we don't get to call
		 * XXX wskbd_cnattach() twice.
		 */
		wsdisplay_set_cons_kbd(ofkbd_cngetc, NULL, NULL);
#endif
	}
}

void
ofwoea_consinit(void)
{
	static int initted = 0;

	if (initted)
		return;

	initted = 1;
	cninit();
}
