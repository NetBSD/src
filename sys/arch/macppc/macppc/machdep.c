/*	$NetBSD: machdep.c,v 1.153.2.1 2010/10/22 07:21:25 uebayasi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.153.2.1 2010/10/22 07:21:25 uebayasi Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_altivec.h"
#include "opt_multiprocessor.h"
#include "adb.h"
#include "zsc.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif
 
#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

#include <dev/ofw/openfirm.h>
#include <dev/wsfb/genfbvar.h>

#include <machine/autoconf.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/bus.h>
#include <machine/fpu.h>
#include <powerpc/oea/bat.h>
#include <powerpc/spr.h>
#ifdef ALTIVEC
#include <powerpc/altivec.h>
#endif
#include <powerpc/ofw_cons.h>

#include <arch/powerpc/pic/picvar.h>
#include <macppc/dev/adbvar.h>
#include <macppc/dev/pmuvar.h>
#include <macppc/dev/cudavar.h>

#include "ksyms.h"
#include "pmu.h"
#include "cuda.h"

#ifdef MULTIPROCESSOR
#include <arch/powerpc/pic/ipivar.h>
#endif

struct genfb_colormap_callback gfb_cb;
struct genfb_parameter_callback gpc;

/*
 * this is bogus - we need to read the actual default from the PMU and then
 * put it here
 */
int backlight_level = 200;

static void of_set_palette(void *, int, int, int, int);
static void add_model_specifics(prop_dictionary_t);
static void of_set_backlight(void *, int);
static int of_get_backlight(void *);

void
initppc(u_int startkernel, u_int endkernel, char *args)
{
	ofwoea_initppc(startkernel, endkernel, args);
}

/* perform model-specific actions at initppc() */
void
model_init(void)
{
}

void
consinit(void)
{
	ofwoea_consinit();
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	oea_startup(NULL);
}

/*
 * Crash dump handling.
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

	/*
	 * Enable external interrupts in case someone is rebooting
	 * from a strange context via ddb.
	 */
	mtmsr(mfmsr() | PSL_EE);

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}

#ifdef MULTIPROCESSOR
	/* Halt other CPU */
	ppc_send_ipi(IPI_T_NOTME, PPC_IPI_HALT);
	delay(100000);	/* XXX */
#endif

	splhigh();

	if (!cold && (howto & RB_DUMP))
		dumpsys();

	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		delay(1000000);
#if NCUDA > 0
		cuda_poweroff();
#endif
#if NPMU > 0
		pmu_poweroff();
#endif
#if NADB > 0
		adb_poweroff();
		printf("WARNING: powerdown failed!\n");
#endif
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");

		/* flush cache for msgbuf */
		__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

		ppc_exit();
	}

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

	/* flush cache for msgbuf */
	__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

#if NCUDA > 0
	cuda_restart();
#endif
#if NPMU > 0
	pmu_restart();
#endif
#if NADB > 0
	adb_restart();	/* not return */
#endif
	ppc_exit();
}

#if 0
/*
 * OpenFirmware callback routine
 */
void
callback(void *p)
{
	panic("callback");	/* for now			XXX */
}
#endif

void
copy_disp_props(struct device *dev, int node, prop_dictionary_t dict)
{
	uint32_t temp;
	uint64_t cmap_cb, backlight_cb;
	int have_backlight = 0;

	if (node != console_node) {
		/*
		 * see if any child matches since OF attaches nodes for
		 * each head and /chosen/stdout points to the head
		 * rather than the device itself in this case
		 */
		int sub;

		sub = OF_child(node);
		while ((sub != 0) && (sub != console_node)) {
			sub = OF_peer(sub);
		}
		if (sub != console_node)
			return;
		node = sub;
	}

	prop_dictionary_set_bool(dict, "is_console", 1);
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
	of_to_dataprop(dict, node, "EDID", "EDID");
	add_model_specifics(dict);

	temp = 0;
	if (OF_getprop(node, "ATY,RefCLK", &temp, sizeof(temp)) != 4) {

		OF_getprop(OF_parent(node), "ATY,RefCLK", &temp,
		    sizeof(temp));
	}
	if (temp != 0)
		prop_dictionary_set_uint32(dict, "refclk", temp / 10);

	gfb_cb.gcc_cookie = (void *)console_instance;
	gfb_cb.gcc_set_mapreg = of_set_palette;
	cmap_cb = (uint64_t)&gfb_cb;
	prop_dictionary_set_uint64(dict, "cmap_callback", cmap_cb);

	/* not let's look for backlight control */
	have_backlight = 0;
	if (OF_getprop(node, "backlight-control", &temp, sizeof(temp)) == 4) {
		have_backlight = 1;
	} else if (OF_getprop(OF_parent(node), "backlight-control", &temp, 
		    sizeof(temp)) == 4) {
		have_backlight = 1;
	}
	if (have_backlight) {
		gpc.gpc_cookie = (void *)console_instance;
		gpc.gpc_set_parameter = of_set_backlight;
		gpc.gpc_get_parameter = of_get_backlight;
		backlight_cb = (uint64_t)&gpc;
		prop_dictionary_set_uint64(dict, "backlight_callback", 
		    backlight_cb);
		/*
		 * since we don't know how to read the backlight level without
		 * access to the PMU we just set it to the default defined
		 * above so the hotkeys work as expected
		 */
		OF_call_method_1("set-contrast", console_instance, 1, 
		    backlight_level);
	}
}

static void
add_model_specifics(prop_dictionary_t dict)
{
	const char *bl_rev_models[] = {
		"PowerBook4,3", "PowerBook6,3", "PowerBook6,5", NULL};
	int node;

	node = OF_finddevice("/");

	if (of_compatible(node, bl_rev_models) != -1) {
		prop_dictionary_set_bool(dict, "backlight_level_reverted", 1);
	}
}

static void
of_set_palette(void *cookie, int index, int r, int g, int b)
{
	int ih = (int)cookie;

	OF_call_method_1("color!", ih, 4, r, g, b, index);
}

static void
of_set_backlight(void *cookie, int level)
{
	int ih = (int)cookie;

	if (level < 0) level = 0;
	if (level > 255) level = 255;
	backlight_level = level;
	OF_call_method_1("set-contrast", ih, 1, level);
}

static int
of_get_backlight(void *cookie)
{

	/*
	 * we don't know how to read the backlight level from OF alone - we
	 * should read the default from the PMU and then just cache whatever
	 * we set last
	 */
	return backlight_level;
}
