/*	$NetBSD: machdep.c,v 1.109 2010/01/20 17:12:08 phx Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.109 2010/01/20 17:12:08 phx Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>
#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/bus.h>
#include <machine/isa_machdep.h>
#include <machine/spr.h>

#include <powerpc/oea/bat.h>
#include <powerpc/ofw_cons.h>
#include <powerpc/rtas.h>

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif
#include "rtas.h"

struct pmap ofw_pmap;
char bootpath[256];

void ofwppc_batinit(void);
void ofppc_bootstrap_console(void);

extern u_int l2cr_config;
#if (NRTAS > 0)
extern int machine_has_rtas;
#endif

struct model_data modeldata;

void
initppc(u_int startkernel, u_int endkernel, char *args)
{
	ofwoea_initppc(startkernel, endkernel, args);
}

/* perform model-specific actions at initppc() */
void
model_init(void)
{
	int qhandle, phandle, j;

	memset(&modeldata, 0, sizeof(struct model_data));
	/* provide sane defaults */
	for (j=0; j < MAX_PCI_BUSSES; j++) {
		modeldata.pciiodata[j].start = 0x00008000;
		modeldata.pciiodata[j].limit = 0x0000ffff;
	}
	modeldata.ranges_offset = 1;

	if (strncmp(model_name, "FirePower,", 10) == 0) {
		modeldata.ranges_offset = 0;
	}
	if (strcmp(model_name, "MOT,PowerStack_II_Pro4000") == 0) {
		modeldata.ranges_offset = 0;
	}

	/* 7044-270 and 7044-170 */
	if (strncmp(model_name, "IBM,7044", 8) == 0) {
		for (j=0; j < MAX_PCI_BUSSES; j++) {
			modeldata.pciiodata[j].start = 0x00fff000;
			modeldata.pciiodata[j].limit = 0x00ffffff;
		}
	}

	/* Pegasos1, Pegasos2 */
	if (strncmp(model_name, "Pegasos", 7) == 0) {
		static uint16_t modew[] = { 640, 800, 1024, 1280, 0 };
		static uint16_t modeh[] = { 480, 600, 768, 1024, 0 };
		uint32_t width, height, mode, fbaddr;
		char buf[32];
		int i;

		modeldata.ranges_offset = 1;
		modeldata.pciiodata[0].start = 0x00001400;
		modeldata.pciiodata[0].limit = 0x0000ffff;
		
		/* the pegasos doesn't bother to set the L2 cache up*/
		l2cr_config = L2CR_L2PE;
		
		/* fix the device_type property of a graphics card */
		for (qhandle = OF_peer(0); qhandle; qhandle = phandle) {
			if (OF_getprop(qhandle, "name", buf, sizeof buf) > 0
			    && strncmp(buf, "display", 7) == 0) {
				OF_setprop(qhandle, "device_type", "display", 8);
				break;
			}
			if ((phandle = OF_child(qhandle)))
				continue;
			while (qhandle) {
				if ((phandle = OF_peer(qhandle)))
					break;
				qhandle = OF_parent(qhandle);
			}
		}

		/*
		 * Get screen width/height and switch to framebuffer mode.
		 * The default dimensions are: 800 x 600
		 */
		OF_interpret("screen-width", 0, 1, &width);
		if (width == 0)
			width = 800;

		OF_interpret("screen-height", 0, 1, &height);
		if (height == 0)
			height = 600;

		/* find VESA mode */
		for (i = 0, mode = 0; modew[i] != 0; i++) {
			if (modew[i] == width && modeh[i] == height) {
				mode = 0x101 + 2 * i;
				break;
			}
		}
		if (!mode) {
			mode = 0x102;
			width = 800;
			height = 600;
		}

		/* init frame buffer mode */
		sprintf(buf, "%x vesa-set-mode", mode);
		OF_interpret(buf, 0, 0);

		/* set dimensions and frame buffer address in OFW */
		sprintf(buf, "%x to screen-width", width);
		OF_interpret(buf, 0, 0);
		sprintf(buf, "%x to screen-height", height);
		OF_interpret(buf, 0, 0);
		OF_interpret("vesa-frame-buffer-adr", 0, 1, &fbaddr);
		if (fbaddr != 0) {
			sprintf(buf, "%x to frame-buffer-adr", fbaddr);
			OF_interpret(buf, 0, 0);
		}
	}
}

void
cpu_startup(void)
{
	oea_startup(model_name[0] ? model_name : NULL);
	bus_space_mallocok();
}


void
consinit(void)
{
	ofwoea_consinit();
}


void
dumpsys(void)
{
	aprint_normal("dumpsys: TBD\n");
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
#if (NRTAS > 0)
	int junk;
#endif

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();         /* sync */
		resettodr();            /* set wall clock */
	}
	splhigh();
	if (howto & RB_HALT) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
		aprint_normal("halted\n\n");
#if (NRTAS > 0)
		if ((howto & 0x800) && machine_has_rtas &&
		    rtas_has_func(RTAS_FUNC_POWER_OFF))
			rtas_call(RTAS_FUNC_POWER_OFF, 2, 1, 0, 0, &junk);
#endif
		ppc_exit();
	}
	if (!cold && (howto & RB_DUMP))
		oea_dumpsys();
	doshutdownhooks();

	pmf_system_shutdown(boothowto);
	aprint_normal("rebooting\n\n");

#if (NRTAS > 0)
	if (machine_has_rtas && rtas_has_func(RTAS_FUNC_SYSTEM_REBOOT)) {
		rtas_call(RTAS_FUNC_SYSTEM_REBOOT, 0, 1, &junk);
		for(;;);
	}
#endif
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			aprint_normal("boot string too large, ignored\n");
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
ofppc_init_comcons(int isa_node)
{
#if (NCOM > 0)
	char name[64];
	uint32_t reg[2], comfreq;
	uint8_t dll, dlm;
	int speed, rate, err, com_node, child;
	bus_space_handle_t comh;

	/* if we have a serial cons, we have work to do */
	memset(name, 0, sizeof(name));
	OF_getprop(console_node, "device_type", name, sizeof(name));
	if (strcmp(name, "serial") != 0)
		return;

	/* scan ISA children for serial devices to match our console */
	com_node = -1;
	for (child = OF_child(isa_node); child; child = OF_peer(child)) {
		memset(name, 0, sizeof(name));
		OF_getprop(child, "device_type", name, sizeof(name));
		if (strcmp(name, "serial") == 0) {
			/*
			 * Serial device even matches our console_node?
			 * Then we're done!
			 */
			if (child == console_node) {
				com_node = child;
				break;
			}
			/* remember first serial device found */
			if (com_node == -1)
				com_node = child;
		}
	}

	if (com_node == -1)
		return;

	if (OF_getprop(com_node, "reg", reg, sizeof(reg)) == -1)
		return;

	if (OF_getprop(com_node, "clock-frequency", &comfreq, 4) == -1)
		comfreq = 0;

	if (comfreq == 0)
		comfreq = COM_FREQ;

	/* we need to BSM this, and then undo that before calling
	 * comcnattach.
	 */

	if (bus_space_map(&genppc_isa_io_space_tag, reg[1], 8, 0, &comh) != 0)
		panic("Can't map isa serial\n");

	bus_space_write_1(&genppc_isa_io_space_tag, comh, com_cfcr, LCR_DLAB);
	dll = bus_space_read_1(&genppc_isa_io_space_tag, comh, com_dlbl);
	dlm = bus_space_read_1(&genppc_isa_io_space_tag, comh, com_dlbh);
	rate = dll | (dlm << 8);
	bus_space_write_1(&genppc_isa_io_space_tag, comh, com_cfcr, LCR_8BITS);
	speed = divrnd((comfreq / 16), rate);
	err = speed - (speed + 150)/300 * 300;
	speed -= err;
	if (err < 0)
		err = -err;
	if (err > 50)
		speed = 9600;

	bus_space_unmap(&genppc_isa_io_space_tag, comh, 8);

	/* Now we can attach the comcons */
	aprint_verbose("Switching to COM console at speed %d", speed);
	if (comcnattach(&genppc_isa_io_space_tag, reg[1],
	    speed, comfreq, COM_TYPE_NORMAL,
	    ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)))
		panic("Can't init serial console");
	aprint_verbose("\n");
#endif /*NCOM*/
}

void
copy_disp_props(struct device *dev, int node, prop_dictionary_t dict)
{
	uint32_t temp;
	char typestr[32];

	memset(typestr, 0, sizeof(typestr));
	OF_getprop(console_node, "device_type", typestr, sizeof(typestr));
	if (strcmp(typestr, "serial") != 0) {
		/* this is our console, when we don't have a serial console */
		prop_dictionary_set_bool(dict, "is_console", 1);
	}

	if (!of_to_uint32_prop(dict, node, "width", "width")) {

		OF_interpret("screen-width", 0, 1, &temp);
		prop_dictionary_set_uint32(dict, "width", temp);
	}
	if (!of_to_uint32_prop(dict, node, "height", "height")) {

		OF_interpret("screen-height", 0, 1, &temp);
		prop_dictionary_set_uint32(dict, "height", temp);
	}
	of_to_uint32_prop(dict, node, "linebytes", "linebytes");
	if (!of_to_uint32_prop(dict, node, "depth", "depth")) {
		/*
		 * XXX we should check linebytes vs. width but those
		 * FBs that don't have a depth property ( /chaos/control... )
		 * won't have linebytes either
		 */
		prop_dictionary_set_uint32(dict, "depth", 8);
	}
	if (!of_to_uint32_prop(dict, node, "address", "address")) {
		uint32_t fbaddr = 0;
			OF_interpret("frame-buffer-adr", 0, 1, &fbaddr);
		if (fbaddr != 0)
			prop_dictionary_set_uint32(dict, "address", fbaddr);
	}
}
