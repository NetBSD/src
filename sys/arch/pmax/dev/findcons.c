/*	$NetBSD: findcons.c,v 1.22 1999/12/23 15:34:18 ad Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone
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
 *      This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: findcons.c,v 1.22 1999/12/23 15:34:18 ad Exp $$");

#include <sys/param.h>
#include <sys/systm.h>
#include <dev/cons.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/machdep.h>

/*
 * Default consdev, for errors or warnings before
 * consinit runs: use the PROM.
 */
struct consdev cd;

#ifndef integrate
#define integrate static inline
#endif


/*
 * Kernel configuration dependencies.
 */
#include "px.h"
#include "pm.h"
#include "cfb.h"
#include "mfb.h"
#include "xcfb.h"
#include "sfb.h"
#include "dc.h"
#include "dtop.h"
#include "scc.h"
#include "tc.h"
#include "rasterconsole.h"

#include <dev/tc/tcvar.h>		/* find TC fraembuffer device. */

#include <machine/tc_machdep.h>

#include <pmax/pmax/asic.h>		/* scc serial console addresses */
#include <pmax/pmax/kn01.h>
#include <pmax/pmax/kn03.h>
#include <pmax/pmax/kmin.h>
#include <pmax/pmax/maxine.h>

#include <machine/pmioctl.h>
#include <machine/fbio.h>		/* framebuffer decls used below */
#include <machine/fbvar.h>
#include <pmax/dev/fbreg.h>

#include <pmax/dev/lk201var.h>
#include <pmax/dev/rconsvar.h>

/* pmax serial interface */
#if NDC > 0
#include <machine/dc7085cons.h>
#include <pmax/dev/dc_cons.h>
#include <pmax/dev/dc_ds_cons.h>
#endif

/* MAXINE desktop bus keyboard */
#if NDTOP > 0
#include <pmax/dev/dtopvar.h>
#endif

/* ioasic serial console */
#if NSCC > 0
#include <pmax/tc/sccvar.h>
#endif

/* ds3100 baseboard framebuffer */
#if NPM > 0
#include <pmax/dev/pmvar.h>
#endif

/* maxine baseboard framebuffer */
#if NXCFB > 0
#include <pmax/dev/xcfbvar.h>
#endif

#define NWS	 (NXCFB + NPM + NMFB + NSFB + NCFB + NPX)

/*
 *  XXX Major device numbers for possible console devices.
 */
#define	DCDEV		16
#define	SCCDEV		17

/*
 * forward declarations
 */

/*
 * device scan functions.
 * This would use the autoconfigatin hiearchy, except we need
 * to pick a console early in boot before autoconfig or
 * malloc are up.
 */
integrate int	dc_ds_kbd	__P((int prom_slot));
integrate int	scc_kbd		__P((int prom_slot));
integrate int	dtop_kbd	__P((int prom_slot));

integrate int	pm_screen	__P((int prom_slot));
integrate int	xcfb_screen	__P((int prom_slot));
#if NRASTERCONSOLE > 0
extern int	tcfb_cnattach	__P((int prom_slot));
#endif

integrate int	dc_ds_serial	__P((int prom_slot));
integrate int	scc_serial	__P((int prom_slot));

int	find_kbd	__P((int prom_slot));
int	find_screen	__P((int prom_slot));
int	find_serial	__P((int prom_slot));

extern struct consdev promcd;

/*
 * Keyboard physically present and driver configured on 3100?
 */
int
dc_ds_kbd(kbd_slot)
	int kbd_slot;
{

#if NDC > 0 && NWS > 0
	if (systype == DS_PMAX) {
		cd.cn_getc = lk_getc;
		lk_divert(dcGetc, makedev(DCDEV, DCKBD_PORT));
		return 1;
	}
	else if (systype == DS_3MAX && kbd_slot == 7) {
		cd.cn_dev = makedev(DCDEV, DCKBD_PORT);
		cd.cn_getc = lk_getc;
		lk_divert(dcGetc, makedev(DCDEV, DCKBD_PORT));
		return 1;
	}
#endif
	return 0;
}

/*
 *  Keyboard configured and physically presnt on 3min, 3maxplus?
 */
int
scc_kbd(kbd_slot)
	int kbd_slot;
{
#if NSCC > 0 && NWS > 0
	if (kbd_slot == 3) {
		cd.cn_dev =  makedev(SCCDEV, SCCKBD_PORT);
		lk_divert(sccGetc, makedev(SCCDEV, SCCKBD_PORT));
		cd.cn_getc = lk_getc;
		return 1;
	}
#endif /* NSCC */
	return 0;
}


/*
 *  Keyboard physically present and driver configured on MAXINE?
 */
int
dtop_kbd(kbd_slot)
	int kbd_slot;
{
	if (systype != DS_MAXINE)
		return 0;
#if  NDTOP > 0
	if (kbd_slot == 3) {
		cd.cn_getc = dtopKBDGetc;
		return 1;
	}
#endif
	return 0;
}



/*
 * Select appropriate  keyboard.
 */
int
find_kbd(kbd)
	int kbd;
{
	switch(systype) {
	case DS_PMAX:
	case DS_3MAX:
		return (dc_ds_kbd(kbd));
		break;

	case DS_3MIN:
	case DS_3MAXPLUS:
		/* keyboard on scc? */
		return (scc_kbd(kbd));
		break;

	case DS_MAXINE:
		return (dtop_kbd(kbd));

	default:
		break;
	}

	/* No keyboard found. */
	return (0);
}


/*
 * Test if screens configured.
 */

int
pm_screen(crtslot)
	int crtslot;
{
#if NPM > 0
	struct fbinfo *fi;
	caddr_t base;
	
	base = (caddr_t)MIPS_PHYS_TO_KSEG1(KN01_SYS_PCC);
	
	if (fballoc(base, &fi))
		return (0);
	
	if (pminit(fi, base, 0, 1)) {
		cd.cn_pri = CN_INTERNAL;
		return (1);
	}
#endif
	return (0);
}


/*
 * Look for MAXINE baseboard video.
 * If selected as console, take it.
 */
int
xcfb_screen(crtslot)
	int crtslot;
{
#if NXCFB > 0
	struct fbinfo *fi;
	caddr_t base;
#endif
	if (systype != DS_MAXINE)
		return 0;
#if NXCFB > 0
	
	base = (caddr_t)MIPS_PHYS_TO_KSEG1(XINE_PHYS_CFB_START);
	
	if (fballoc(base, &fi))
		return (0);

	if (crtslot == 3 && xcfbinit(fi, base, 0, 1)) {
		cd.cn_pri = CN_INTERNAL;
		return(1);
	} else
#endif
	return (0);
}

/*
 * Look for screen.
 */
int
find_screen(crtslot)
	int crtslot;
{
	switch(systype) {
	case DS_PMAX:
		if (pm_screen(crtslot))
			return (1);
		/* no other options on 3100s. */
		break;

	case DS_MAXINE:
		if (crtslot == 3 && xcfb_screen(crtslot))
			return (1);

	/*FALLTHROUGH*/
	case DS_3MAX:
	case DS_3MIN:
	case DS_3MAXPLUS:
#if NRASTERCONSOLE > 0
		/* TC option video?*/
		if (tcfb_cnattach(crtslot))
			return(1);
#endif
		break;
	default:
		break;
	}

	/* no screen found. */
	return (0);
}

int
dc_ds_serial(comslot)
	int comslot;
{
#if NDC > 0
	if (comslot == 4)
		cd.cn_dev = makedev(DCDEV, DCCOMM_PORT);
	else
	if (comslot == 0) {
		cd.cn_dev = makedev(DCDEV, 0);
	} else
		cd.cn_dev = makedev(DCDEV, DCPRINTER_PORT);
	return dc_ds_consinit(cd.cn_dev);
#endif
	return 0;
}

int
scc_serial(comslot)
	int comslot;
{
#if NSCC > 0
	int dev;
	void * sccaddr;

	/*
	 * On the 3min and 3maxplus, the default serial-console
	 * port is on the chip at the higher address.
	 * On the MAXINE, there is only  serial port, which
	 * configures at the lower address.
	 */
	switch (systype) {
	case DS_MAXINE:
		dev = SCCCOMM2_PORT;
		sccaddr = (void*)(TC_KV(XINE_SYS_ASIC) + IOASIC_SLOT_4_START);
		break;

	case DS_3MIN:
		dev = SCCCOMM3_PORT;
		sccaddr = (void*)(TC_KV(KMIN_SYS_ASIC) + IOASIC_SLOT_6_START);
		break;

	case DS_3MAXPLUS:
		dev = SCCCOMM3_PORT;
		sccaddr = (void*)(TC_KV(KN03_SYS_ASIC) + IOASIC_SLOT_6_START);
		break;
	default:
		return (0);
	}

	cd.cn_dev = makedev(SCCDEV, dev);

	scc_consinit(cd.cn_dev, sccaddr);
	return 1;
#endif /* NSCC */

	return 0;
}

/*
 * find a serial console.
 */
int
find_serial(comslot)
	int comslot;
{
	switch(systype) {
	case DS_PMAX:
	case DS_3MAX:
		return (dc_ds_serial(comslot));
		break;

	case DS_MIPSMATE:
		/* console fixed on line 0 */
		return (dc_ds_serial(0));
		break;

	case DS_MAXINE: /* XXX only one serial port */
	case DS_3MIN:
	case DS_3MAXPLUS:
		return (scc_serial(comslot));
		break;

	default:
		break;
	}

	return (0);
}


/*
 * Pick a console.
 * Use the same devices as PROM says, if we have a driver.
 * if wedont' have a driver for the device the PROM chose,
 * pick the highest-priority device according to the Owners Guide rules.
 * If no match,  stick with the PROM and hope for the best.
 */
void
consinit()
{
	int prom_using_screen = 0;	/* boolean */
	int kbd = 0;			/* prom kbd locator */
	int crt = 0;			/* prom framebuffer locator */

	/* Use copy of PROM console until we find something better. */
	cd = promcd;
	cn_tab = &cd;

	/* get PROM's idea of what the console is. */
	prom_findcons(&kbd, &crt, &prom_using_screen);


	/*  if the prom is using a screen, try that. */
	if (prom_using_screen) {
		if (find_kbd(kbd) && find_screen(crt)) {
#if NRASTERCONSOLE > 0
			cd.cn_pri = CN_NORMAL;
			rcons_indev(&cd);
			return;
#endif /* NRASTERCONSOLE > 0 */

		}

		/* PROM is using fb console, but we have no driver for it. */
		printf("No supported console device in slot %d. ", crt);
	    	printf("Trying serial console.\n");
		DELAY(500000);
	} else {
		/*
		 * If we found a keyboard and framebuffer, wire it up
		 * as an rcons device even if it's not the OS console.
		 */
	}

	/* Otherwise, try a serial port as console. */
	if (find_serial(kbd)) {
		cd.cn_pri = CN_REMOTE;
		return;
	}

	/*
	 * Nothing worked. Revert to using PROM.
	 * Maybe some device will usurp the console during
	 * autoconfiguration.
	 */
	cd = promcd;

	printf("No driver for console device, using prom for console I/O.\n");
	DELAY(500000);
	return;
}
