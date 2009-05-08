/*	$NetBSD: autoconf.c,v 1.29 2009/05/08 09:33:58 skrll Exp $	*/

/*	$OpenBSD: autoconf.c,v 1.15 2001/06/25 00:43:10 mickey Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)autoconf.c	8.4 (Berkeley) 10/1/93
 */

/*
 * Copyright (c) 1998-2001 Michael Shalayeff
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)autoconf.c	8.4 (Berkeley) 10/1/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.29 2009/05/08 09:33:58 skrll Exp $");

#include "opt_kgdb.h"
#include "opt_useleds.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/callout.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/pci/pcivar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/cons.h>

#include <hp700/hp700/machdep.h>
#include <hp700/dev/cpudevs.h>
#include <hp700/gsc/gscbusvar.h>

register_t	kpsw = PSW_Q | PSW_P | PSW_C | PSW_D;

/*
 * LED blinking thing
 */
#ifdef USELEDS
int _hp700_led_on_cycles[_HP700_LEDS_BLINKABLE];
static struct callout hp700_led_callout;
static void hp700_led_blinker(void *);
extern int hz;
#endif

void (*cold_hook)(int); /* see below */

/*
 * cpu_configure:
 * called at boot time, configure all devices on system
 */
void
cpu_configure(void)
{

	/*
	 * Consider stopping for a debugger before
	 * autoconfiguration.
	 */
	if (boothowto & RB_KDB) {
#ifdef KGDB
		extern int hp700_kgdb_attached;
		if (hp700_kgdb_attached)
			kgdb_connect(1);
#elif defined(DDB)
		Debugger();
#endif	/* DDB */
	}

	splhigh();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* in spl*() we trust */
	hp700_intr_init();
	__asm volatile("ssm %0, %%r0" :: "i" (PSW_I));
	kpsw |= PSW_I;
	spl0();

	if (cold_hook)
		(*cold_hook)(HPPA_COLD_HOT);

#ifdef USELEDS
	memset(_hp700_led_on_cycles, 0, sizeof(_hp700_led_on_cycles));
	callout_init(&hp700_led_callout, 0);
	hp700_led_blinker((void *) 0);
#endif
}

#ifdef USELEDS
/*
 * This sets LEDs.
 */
void
hp700_led_ctl(int off, int on, int toggle)
{
	int r;

	if (machine_ledaddr == NULL)
		return;

	/* The mask is reversed when pushed out to the hardware. */
	r = ~(machine_leds = ((machine_leds & ~off) | on) ^ toggle);

	if (machine_ledword)
		*machine_ledaddr = r;
	else {
#define	HP700_LED_DATA		0x01
#define	HP700_LED_STROBE	0x02
		int b;
		for (b = 0x80; b; b >>= 1) {
			*machine_ledaddr = (r & b)? HP700_LED_DATA : 0;
			DELAY(1);
			*machine_ledaddr = ((r & b)? HP700_LED_DATA : 0) |
			    HP700_LED_STROBE;
		}
#undef	HP700_LED_DATA
#undef	HP700_LED_STROBE
	}
}

/*
 * This callout handler blinks LEDs.
 */
static void
hp700_led_blinker(void *arg)
{
	u_int led_cycle = (u_int) arg;
	int leds, led_i, led;
	int load;

	/*
	 * Blink the heartbeat LED like this:
	 *
	 *   |~| |~|
	 *  _| |_| |_,_,_,_
	 *   0 1 2 3 4 6 7
	 */
#define HP700_HEARTBEAT_CYCLES	(_HP700_LED_FREQ / 8)
	if (led_cycle == (0 * HP700_HEARTBEAT_CYCLES) ||
	    led_cycle == (2 * HP700_HEARTBEAT_CYCLES)) {
		_hp700_led_on_cycles[HP700_LED_HEARTBEAT] =
			HP700_HEARTBEAT_CYCLES;
	}

	/* Form the new LED mask. */
	leds = 0;
	for (led_i = 0, led = (1 << 0);
	     led_i < _HP700_LEDS_BLINKABLE;
	     led_i++, led <<= 1) {
		if (_hp700_led_on_cycles[led_i] > 0)
			leds |= led;
		if (_hp700_led_on_cycles[led_i] >= 0)
			_hp700_led_on_cycles[led_i]--;
	}

	/* Add in the system load. */
	load = averunnable.ldavg[0] >> FSHIFT;
	if (load >= (1 << (_HP700_LEDS_COUNT - _HP700_LEDS_BLINKABLE)))
		load = (1 << (_HP700_LEDS_COUNT - _HP700_LEDS_BLINKABLE)) - 1;
	leds |= (load << _HP700_LEDS_BLINKABLE);

	/* Set the LEDs. */
	hp700_led_ctl(-1, leds, 0);
	
	/* NB: this assumes _HP700_LED_FREQ is a power of two. */
	led_cycle = (led_cycle + 1) & (_HP700_LED_FREQ - 1);
	callout_reset(&hp700_led_callout, hz / _HP700_LED_FREQ,
		hp700_led_blinker, (void *) led_cycle);
	
}
#endif /* USELEDS */

/*
 * This is called by configure to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	const struct bdevsw *bdev;
	extern int dumpsize;
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		dumpdev = NODEV;
		goto bad;
	}
	if (bdev->d_psize == NULL)
		goto bad;
	nblks = (*bdev->d_psize)(dumpdev);
	if (nblks <= ctod(1))
		goto bad;
	dumpblks = cpu_dumpsize();
	if (dumpblks < 0)
		goto bad;
	dumpblks += ctod(physmem);

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1)))
		goto bad;

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = physmem;
	return;

bad:
	dumpsize = 0;
	return;
}

/****************************************************************/

device_t boot_device = NULL;


void
device_register(device_t dev, void *aux)
{
	int pagezero_cookie;
	device_t pdev;

	if ((pdev = device_parent(dev)) == NULL ||
	    device_parent(pdev) == NULL)
		return;
	pagezero_cookie = hp700_pagezero_map();

	/*
	 * The boot device is described in PAGE0->mem_boot. We need to do it
	 * this way as the MD device path (DP) information in struct confargs
	 * is only available in hp700 MD devices. So boot_device is used to
	 * propagate information down the device tree.
	 *
	 * If the boot device is a GSC network device all we need to compare
	 * is the HPA or device path (DP) to get the boot device.
	 * If the boot device is a SCSI device below a GSC attached SCSI
	 * controller PAGE0->mem_boot.pz_hpa contains the HPA of the SCSI
	 * controller. In that case we remember the the pointer to the
	 * controller's struct dev in boot_device. The SCSI device is located
	 * later, see below.
	 */
	if ((device_is_a(pdev, "gsc") || device_is_a(pdev, "phantomas"))
	    && (hppa_hpa_t)PAGE0->mem_boot.pz_hpa ==
	    ((struct gsc_attach_args *)aux)->ga_ca.ca_hpa)
		/* This is (the controller of) the boot device. */
		boot_device = dev;
	/*
	 * If the boot device is a PCI device the HPA is the address where the
	 * firmware has maped the PCI memory of the PCI device. This is quite
	 * device dependent, so we compare the DP. It encodes the bus routing
	 * information to the PCI bus bridge in the DP head and the PCI device
	 * and PCI function in the last two DP components. So we compare the
	 * head of the DP when a PCI bridge attaches and remember the struct
	 * dev of the PCI bridge in boot_dev if it machtes. Later, when PCI
	 * devices are attached, we look if this PCI device hangs below the
	 * boot PCI bridge. If yes we compare the PCI device and PCI function
	 * to the DP tail. In case of a network boot we found the boot device
	 * on a match. In case of a SCSI boot device we have to do the same
	 * check when SCSI devices are attached like on GSC SCSI controllers.
	 */
	if (device_is_a(dev, "dino")) {
		struct confargs *ca = (struct confargs *)aux;
		int i, n;

		for (n = 0 ; ca->ca_dp.dp_bc[n] < 0 ; n++) {
			/* Skip unused DP components. */
		}
		for (i = 0 ; i < 6 && n < 6 ; i++) {
			/* Skip unused DP components... */
			if (PAGE0->mem_boot.pz_dp.dp_bc[i] < 0)
				continue;
			/* and compare the rest. */
			if (PAGE0->mem_boot.pz_dp.dp_bc[i]
			    != ca->ca_dp.dp_bc[n]) {
				hp700_pagezero_unmap(pagezero_cookie);
				return;
			}
			n++;
		}
		if (PAGE0->mem_boot.pz_dp.dp_bc[i] != ca->ca_dp.dp_mod) {
			hp700_pagezero_unmap(pagezero_cookie);
			return;
		}
		/* This is the PCI host bridge in front of the boot device. */
		boot_device = dev;
	}
	/* XXX Guesswork. No hardware to test how firmware handles a ppb. */
	if (device_is_a(dev, "ppb")
	    && boot_device == device_parent(pdev)
	    && ((struct pci_attach_args*)aux)->pa_device
	    == PAGE0->mem_boot.pz_dp.dp_bc[3]
	    && ((struct pci_attach_args*)aux)->pa_function
	    == PAGE0->mem_boot.pz_dp.dp_bc[4])
		/* This is the PCI - PCI bridge in front of the boot device. */
		boot_device = dev;
	if (device_is_a(pdev, "pci")
	    && boot_device == device_parent(pdev)
	    && ((struct pci_attach_args*)aux)->pa_device
	    == PAGE0->mem_boot.pz_dp.dp_bc[5]
	    && ((struct pci_attach_args*)aux)->pa_function
	    == PAGE0->mem_boot.pz_dp.dp_mod)
		/* This is (the controller of) the boot device. */
		boot_device = dev;
	/*
	 * When SCSI devices are attached, we look if the SCSI device hangs
	 * below the controller remembered in boot_device. If so, we compare
	 * the SCSI ID and LUN with the DP layer information. If they match
	 * we found the boot device.
	 */
	if (device_is_a(pdev, "scsibus")
	    && boot_device == device_parent(pdev)
	    && ((struct scsipibus_attach_args *)aux)->sa_periph->periph_target
	    == PAGE0->mem_boot.pz_dp.dp_layers[0]
	    && ((struct scsipibus_attach_args *)aux)->sa_periph->periph_lun
	    == PAGE0->mem_boot.pz_dp.dp_layers[1])
		/* This is the boot device. */
		boot_device = dev;

	hp700_pagezero_unmap(pagezero_cookie);
	return;
}

/*
 * Choose root and swap devices.
 */
void
cpu_rootconf(void)
{
#ifdef DEBUG
	int pagezero_cookie;
	int n;

	pagezero_cookie = hp700_pagezero_map();
	printf("PROM boot device: hpa %p path ", PAGE0->mem_boot.pz_hpa);
	for (n = 0 ; n < 6 ; n++) {
		if (PAGE0->mem_boot.pz_dp.dp_bc[n] >= 0)
			printf("%d/", PAGE0->mem_boot.pz_dp.dp_bc[n]);
	}
	printf("%d dp_layers ", PAGE0->mem_boot.pz_dp.dp_mod);
	for (n = 0 ; n < 6 ; n++) {
		printf( "0x%x%c", PAGE0->mem_boot.pz_dp.dp_layers[n],
		    n < 5 ? '/' : ' ');
	}
	printf("dp_flags 0x%x pz_class 0x%x\n", PAGE0->mem_boot.pz_dp.dp_flags,
	    PAGE0->mem_boot.pz_class);
#endif /* DEBUG */

	if (boot_device != NULL)
		printf("boot device: %s\n", boot_device->dv_xname );
	setroot(boot_device, 0);
}


#ifdef RAMDISK_HOOKS
/*static struct device fakerdrootdev = { DV_DISK, {}, NULL, 0, "rd0", NULL };*/
#endif

static struct pdc_memmap pdc_memmap PDC_ALIGNMENT;
static struct pdc_iodc_read pdc_iodc_read PDC_ALIGNMENT;
static struct pdc_system_map_find_mod pdc_find_mod PDC_ALIGNMENT;
static struct pdc_system_map_find_addr pdc_find_addr PDC_ALIGNMENT;

void
pdc_scanbus(device_t self, struct confargs *ca,
    void (*callback)(device_t, struct confargs *))
{
	int i;

	for (i = 0; i < ca->ca_nmodules; i++) {
		struct confargs nca;
		int error;

		memset(&nca, 0, sizeof(nca));
		nca.ca_iot = ca->ca_iot;
		nca.ca_dmatag = ca->ca_dmatag;
		nca.ca_dp.dp_bc[0] = ca->ca_dp.dp_bc[1];
		nca.ca_dp.dp_bc[1] = ca->ca_dp.dp_bc[2];
		nca.ca_dp.dp_bc[2] = ca->ca_dp.dp_bc[3];
		nca.ca_dp.dp_bc[3] = ca->ca_dp.dp_bc[4];
		nca.ca_dp.dp_bc[4] = ca->ca_dp.dp_bc[5];
		nca.ca_dp.dp_bc[5] = ca->ca_dp.dp_mod;
		nca.ca_dp.dp_mod = i;
		nca.ca_naddrs = 0;
		nca.ca_hpa = 0;

		if (ca->ca_hpabase) {
			nca.ca_hpa = ca->ca_hpabase + IOMOD_HPASIZE * i;
			nca.ca_dp.dp_mod = i;
		} else if ((error = pdc_call((iodcio_t)pdc, 0, PDC_MEMMAP,
		    PDC_MEMMAP_HPA, &pdc_memmap, &nca.ca_dp)) == 0)
			nca.ca_hpa = pdc_memmap.hpa;
		else if ((error = pdc_call((iodcio_t)pdc, 0, PDC_SYSTEM_MAP,
		    PDC_SYSTEM_MAP_TRANS_PATH, &pdc_memmap, &nca.ca_dp)) == 0) {
			struct device_path path;
			int im, ia;

			nca.ca_hpa = pdc_memmap.hpa;

			for (im = 0; !(error = pdc_call((iodcio_t)pdc, 0,
			    PDC_SYSTEM_MAP, PDC_SYSTEM_MAP_FIND_MOD,
			    &pdc_find_mod, &path, im)) &&
			    pdc_find_mod.hpa != nca.ca_hpa; im++)
				;

			if (!error)
				nca.ca_hpasz = pdc_find_mod.size << PGSHIFT;

			if (!error && pdc_find_mod.naddrs) {
				nca.ca_naddrs = pdc_find_mod.naddrs;
				if (nca.ca_naddrs > 16) {
					nca.ca_naddrs = 16;
					printf("WARNING: too many (%d) addrs\n",
					    pdc_find_mod.naddrs);
				}

				for (ia = 0; !(error = pdc_call((iodcio_t)pdc,
				    0, PDC_SYSTEM_MAP, PDC_SYSTEM_MAP_FIND_ADDR,
				    &pdc_find_addr, im, ia + 1)) && ia < nca.ca_naddrs; ia++) {
					nca.ca_addrs[ia].addr = pdc_find_addr.hpa;
					nca.ca_addrs[ia].size =
					    pdc_find_addr.size << PGSHIFT;

				}
			}
		}

		if (!nca.ca_hpa)
			continue;

		if ((error = pdc_call((iodcio_t)pdc, 0, PDC_IODC,
		    PDC_IODC_READ, &pdc_iodc_read, nca.ca_hpa, IODC_DATA,
		    &nca.ca_type, sizeof(nca.ca_type))) < 0)
			continue;

		nca.ca_irq = HP700CF_IRQ_UNDEF;
		nca.ca_pdc_iodc_read = &pdc_iodc_read;
		nca.ca_name = hppa_mod_info(nca.ca_type.iodc_type,
		    nca.ca_type.iodc_sv_model);

		(*callback)(self, &nca);
	}

}

static const struct hppa_mod_info hppa_knownmods[] = {
#include <hp700/dev/cpudevs_data.h>
};

const char *
hppa_mod_info(int type, int sv)
{
	const struct hppa_mod_info *mi;
	static char fakeid[32];

	for (mi = hppa_knownmods; mi->mi_type >= 0 &&
	     (mi->mi_type != type || mi->mi_sv != sv); mi++);

	if (mi->mi_type < 0) {
		sprintf(fakeid, "type %x, sv %x", type, sv);
		return fakeid;
	} else
		return mi->mi_name;
}
