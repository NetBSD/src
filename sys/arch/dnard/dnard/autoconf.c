/*	$NetBSD: autoconf.c,v 1.1 2001/05/09 15:58:07 matt Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Mark Brinicombe for
 *      the NetBSD project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * autoconf.c
 *
 * Autoconfiguration functions
 *
 * Created      : 08/10/94
 */

#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/termios.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#include <dev/ic/comvar.h>

#include <dnard/dnard/sequoia.h>

extern void	startrtclock __P((void));
extern int	vga_ofbus_cnattach(int, bus_space_tag_t, bus_space_tag_t);

extern char *boot_file;
extern char *boot_kernel;
struct device *booted_device;
int booted_partition;

extern dev_t dumpdev;

void dumpconf __P((void));
void isa_intr_init __P((void));

/*
 * Set up the root device from the boot args
 */
void
cpu_rootconf()
{
#ifndef MEMORY_DISK_IS_ROOT
	printf("boot device: %s\n",
	    booted_device != NULL ? booted_device->dv_xname : "<unknown>");
#endif
	setroot(booted_device, booted_partition);
}


/*
 * void cpu_configure()
 *
 * Configure all the root devices
 * The root devices are expected to configure their own children
 */

void
cpu_configure()
{
	int node;
	struct ofbus_attach_args aa;

	/*
	 * Configure all the roots.
	 */

	sequoiaInit();
	startrtclock();

	/*
	 * Since the ICU is not standard on the ARM we don't know
	 * if we have one until we find a bridge.
	 * Since various PCI interrupts could be routed via the ICU
	 * (for PCI devices in the bridge) we need to set up the ICU
	 * now so that these interrupts can be established correctly
	 * i.e. This is a hack.
	 */
#if 0
#include "isa.h"
#if NISA > 0 && !defined(SHARK)
	isa_intr_init();
#endif
#endif

	/*
	 *  Walk the OFW device tree and configure found devices.
	 */

	if (!(node = OF_peer(0)))
		panic("No OFW root");
	aa.oba_busname = "ofw";
	aa.oba_phandle = node;
	if (!config_rootfound("ofbus", &aa))
		panic("ofw root ofbus not configured");

	/* Debugging information */
#ifndef TERSE
	printf("ipl_bio=%08x ipl_net=%08x ipl_tty=%08x ipl_imp=%08x\n",
	    irqmasks[IPL_BIO], irqmasks[IPL_NET], irqmasks[IPL_TTY],
	    irqmasks[IPL_IMP]);
	printf("ipl_audio=%08x ipl_imp=%08x ipl_high=%08x ipl_serial=%08x\n",
	    irqmasks[IPL_AUDIO], irqmasks[IPL_CLOCK], irqmasks[IPL_HIGH],
	    irqmasks[IPL_SERIAL]);
#endif

	/* Time to start taking interrupts so lets open the flood gates .... */
	(void)spl0();
}

void
device_register(struct device *dev, void *aux)
{
	static struct device *parent;
#if NSD > 0 || NCD > 0
	static struct device *scsipidev;
#endif
	static char *boot_component;
	struct ofbus_attach_args *oba;
	const char *cd_name = dev->dv_cfdata->cf_driver->cd_name;
	char name[64];
	int i;

	if (boot_component == NULL) {
		char *cp;
		boot_component = boot_file;
		if (boot_component == NULL)
			return;
		cp = strrchr(boot_component, ':');
		if (cp != NULL) {
			*cp++ = '\0';
			if (cp[0] == '\\')
				cp++;
			boot_kernel = cp; 
		}
	}

	if (booted_device != NULL
	    || boot_component == NULL
	    || boot_component[0] == '\0')
		return;

	if (!strcmp(cd_name, "ofbus") || !strcmp(cd_name, "ofisa")) {
		oba = aux;
	} else if (parent == NULL) {
		return;
	} else if (parent == dev->dv_parent
		   && !strcmp(parent->dv_cfdata->cf_driver->cd_name, "ofisa")) {
		struct ofisa_attach_args *aa = aux;
		oba = &aa->oba;
#if NWD > 0 || NSD > 0 || NCD > 0
	} else if (parent == dev->dv_parent
		   && !strcmp(parent->dv_cfdata->cf_driver->cd_name, "wdc")) {
#if NSD > 0 || NCD > 0
		if (!strcmp(cd_name, "atapibus")) {
			scsipidev = dev;
			return;
		}
#endif
#if NWD > 0
		if (!strcmp(cd_name, "wd")) {
			struct ata_atapi_attach *aa = aux;
			char *cp = strchr(boot_component, '@');
			if (cp != NULL
			    && aa->aa_drv_data->drive == strtoul(cp+1, NULL, 16)
			    && aa->aa_channel == 0) {
				booted_device = dev;
				return;
			}
		}
		return;
#endif /* NWD > 0 */
#if NSD > 0 || NCD > 0
	} else if (scsipidev == dev->dv_parent
	    && (!strcmp(cd_name, "sd") || !strcmp(cd_name, "cd"))) {
		struct scsipibus_attach_args *sa = aux;
		char *cp = strchr(boot_component, '@');
		if (cp != NULL
		    && sa->sa_sc_link->type == BUS_ATAPI
		    && sa->sa_sc_link->scsipi_atapi.channel == 0
		    && sa->sa_sc_link->scsipi_atapi.drive == strtoul(cp+1, NULL, 16)) {
			booted_device = dev;
		}
		return;
#endif /* NSD > 0 || NCD > 0 */
#endif /* NWD > 0 || NSD > 0 || NCD > 0 */
	} else {
		return;
	}
	(void) of_packagename(oba->oba_phandle, name, sizeof name);
	i = strlen(name);
	if (!strncmp(name, &boot_component[1], i)
	    && (boot_component[i+1] == '/' || boot_component[i+1] == '\0')) {
		boot_component += i + 1;
		if (boot_component[0] == '/') {
			parent = dev;
		} else {
			booted_device = dev;
		}
	}
}

#include "ofb.h"
#include "vga_ofbus.h"
#include "com.h"

#if (NCOM > 0)
#ifndef CONSPEED
#define	CONSPEED	9600
#endif
#ifndef CONMODE
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnmode = CONMODE;
#endif /* (NCOM > 0) */

void
cninit(void)
{
	int chosen, stdin, stdout, node;
	char type[16], name[16];

	chosen = OF_finddevice("/chosen");
	if (chosen == 0)
		goto nocons;
	
	if (OF_getprop(chosen, "stdout", &stdout, sizeof(stdout))
	    != sizeof(stdout))
		goto nocons;

	node = OF_instance_to_package(stdout);
	bzero(type, sizeof(type));
	if (OF_getprop(node, "device_type", type, sizeof(type)) == -1)
		goto nocons;

	if (strcmp(type, "display") == 0) {
#if NOFB > 0
		if (!ofb_cnattach())
			goto dokbd;
#endif
#if NVGA_OFBUS > 0
		if (!vga_ofbus_cnattach(node, &isa_io_bs_tag, &isa_mem_bs_tag))
			goto dokbd;
#endif
		return;
	dokbd:

		/*
		 * We must determine which keyboard type we have.
		 */
		if (OF_getprop(chosen, "stdin", &stdin, sizeof(stdin))
		    != sizeof(stdin)) {
			printf("WARNING: no `stdin' property in /chosen\n");
			return;
		}

		node = OF_instance_to_package(stdin);
		bzero(name, sizeof(name));
		OF_getprop(node, "name", name, sizeof(name));
		if (strcmp(name, "keyboard") != 0) {
			printf("WARNING: stdin is not a keyboard: %s\n", name);
			return;
		}

#if (NPCKBC > 0)
		pckbc_cnattach(&isa_io_bs_tag, IO_KBD, KBCMDP, PCKBC_KBD_SLOT);
#endif
		return;
	}

#if NCOM > 0
	if (strcmp(type, "serial") == 0) {
		int regs[3];
		int freq;
		if (OF_getprop(node, "reg", regs, sizeof(regs)) != sizeof(regs))
			goto nocom;
		if (OF_getprop(node, "clock-frequency", &freq, sizeof(freq)) 
		    != sizeof(freq))
			goto nocom;
		if (!comcnattach(&isa_io_bs_tag, regs[1], CONSPEED, freq,
		    comcnmode))
			return;
	  nocom:
		panic("can't init serial console (hanlde=%#x)", node);
		return;
	}
#endif

nocons:
	return;
}
