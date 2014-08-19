/*	$NetBSD: autoconf.c,v 1.2.10.2 2014/08/20 00:03:04 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.2.10.2 2014/08/20 00:03:04 tls Exp $");

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
#include <sys/kmem.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/pci/pcivar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/cons.h>

#include <hppa/hppa/machdep.h>
#include <hppa/dev/cpudevs.h>
#include <hppa/gsc/gscbusvar.h>

static TAILQ_HEAD(hppa_pdcmodule_head, hppa_pdcmodule) hppa_pdcmodule_list =
    TAILQ_HEAD_INITIALIZER(hppa_pdcmodule_list);

struct hppa_pdcmodule {
	TAILQ_ENTRY(hppa_pdcmodule) hm_link;
	bool			hm_registered;
	struct pdc_iodc_read	hm_pir;
	struct iodc_data	hm_type;
	struct device_path	hm_dp;
	hppa_hpa_t		hm_hpa;
	u_int			hm_hpasz;
	u_int			hm_naddrs;	/* only PDC_SYSTEM_MAP */
	u_int			hm_modindex;	/* only PDC_SYSTEM_MAP */
};

#define	HPPA_SYSTEMMAPMODULES	256

/*
 * LED blinking thing
 */
#ifdef USELEDS
int _hppa_led_on_cycles[_HPPA_LEDS_BLINKABLE];
static struct callout hppa_led_callout;
static void hppa_led_blinker(void *);
extern int hz;
#endif

void (*cold_hook)(int); /* see below */

struct hppa_pdcmodule *hppa_pdcmodule_create(struct hppa_pdcmodule *,
    const char *);
void hppa_walkbus(struct confargs *ca);
static void hppa_pdc_snake_scan(void);
static void hppa_pdc_system_map_scan(void);

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
		extern int hppa_kgdb_attached;
		if (hppa_kgdb_attached)
			kgdb_connect(1);
#elif defined(DDB)
		Debugger();
#endif	/* DDB */
	}

	splhigh();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* Allow interrupts - we're trusting spl* here */
	hppa_intr_enable();
	spl0();

	if (cold_hook)
		(*cold_hook)(HPPA_COLD_HOT);

#ifdef USELEDS
	memset(_hppa_led_on_cycles, 0, sizeof(_hppa_led_on_cycles));
	callout_init(&hppa_led_callout, 0);
	hppa_led_blinker((void *) 0);
#endif
}

#ifdef USELEDS
/*
 * This sets LEDs.
 */
void
hppa_led_ctl(int off, int on, int toggle)
{
	int r;

	if (machine_ledaddr == NULL)
		return;

	/* The mask is reversed when pushed out to the hardware. */
	r = ~(machine_leds = ((machine_leds & ~off) | on) ^ toggle);

	if (machine_ledword)
		*machine_ledaddr = r;
	else {
#define	HPPA_LED_DATA	0x01
#define	HPPA_LED_STROBE	0x02
		int b;
		for (b = 0x80; b; b >>= 1) {
			*machine_ledaddr = (r & b)? HPPA_LED_DATA : 0;
			DELAY(1);
			*machine_ledaddr = ((r & b)? HPPA_LED_DATA : 0) |
			    HPPA_LED_STROBE;
		}
#undef	HPPA_LED_DATA
#undef	HPPA_LED_STROBE
	}
}

/*
 * This callout handler blinks LEDs.
 */
static void
hppa_led_blinker(void *arg)
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
#define HPPA_HEARTBEAT_CYCLES	(_HPPA_LED_FREQ / 8)
	if (led_cycle == (0 * HPPA_HEARTBEAT_CYCLES) ||
	    led_cycle == (2 * HPPA_HEARTBEAT_CYCLES)) {
		_hppa_led_on_cycles[HPPA_LED_HEARTBEAT] =
			HPPA_HEARTBEAT_CYCLES;
	}

	/* Form the new LED mask. */
	leds = 0;
	for (led_i = 0, led = (1 << 0);
	     led_i < _HPPA_LEDS_BLINKABLE;
	     led_i++, led <<= 1) {
		if (_hppa_led_on_cycles[led_i] > 0)
			leds |= led;
		if (_hppa_led_on_cycles[led_i] >= 0)
			_hppa_led_on_cycles[led_i]--;
	}

	/* Add in the system load. */
	load = averunnable.ldavg[0] >> FSHIFT;
	if (load >= (1 << (_HPPA_LEDS_COUNT - _HPPA_LEDS_BLINKABLE)))
		load = (1 << (_HPPA_LEDS_COUNT - _HPPA_LEDS_BLINKABLE)) - 1;
	leds |= (load << _HPPA_LEDS_BLINKABLE);

	/* Set the LEDs. */
	hppa_led_ctl(-1, leds, 0);
	
	/* NB: this assumes _HPPA_LED_FREQ is a power of two. */
	led_cycle = (led_cycle + 1) & (_HPPA_LED_FREQ - 1);
	callout_reset(&hppa_led_callout, hz / _HPPA_LED_FREQ,
		hppa_led_blinker, (void *) led_cycle);
	
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
	extern int dumpsize;
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	nblks = bdev_size(dumpdev);
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
	pagezero_cookie = hppa_pagezero_map();

	/*
	 * The boot device is described in PAGE0->mem_boot. We need to do it
	 * this way as the MD device path (DP) information in struct confargs
	 * is only available in hppa MD devices. So boot_device is used to
	 * propagate information down the device tree.
	 *
	 * If the boot device is a GSC network device all we need to compare
	 * is the HPA or device path (DP) to get the boot device.
	 * If the boot device is a SCSI device below a GSC attached SCSI
	 * controller PAGE0->mem_boot.pz_hpa contains the HPA of the SCSI
	 * controller. In that case we remember the pointer to the
	 * controller's struct dev in boot_device. The SCSI device is located
	 * later, see below.
	 */
	if (device_is_a(pdev, "gsc") || device_is_a(pdev, "phantomas") ||
	    device_is_a(pdev, "uturn")) {
		struct confargs *ca = aux;

		if ((hppa_hpa_t)PAGE0->mem_boot.pz_hpa == ca->ca_hpa) {
			/* This is (the controller of) the boot device. */
			boot_device = dev;
		}
	}
	/*
	 * If the boot device is a PCI device the HPA is the address where the
	 * firmware has mapped the PCI memory of the PCI device. This is quite
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
	if (device_is_a(dev, "dino") || device_is_a(dev, "elroy")) {
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
				hppa_pagezero_unmap(pagezero_cookie);
				return;
			}
			n++;
		}
		if (PAGE0->mem_boot.pz_dp.dp_bc[i] != ca->ca_dp.dp_mod) {
			hppa_pagezero_unmap(pagezero_cookie);
			return;
		}
		/* This is the PCI host bridge in front of the boot device. */
		boot_device = dev;

	}
	if (device_is_a(dev, "ppb") && boot_device == device_parent(pdev)) {
		/*
		 * XXX Guesswork. No hardware to test how firmware handles
		 * a ppb.
		 */
		struct pci_attach_args *paa = (struct pci_attach_args*)aux;
		
		if (paa->pa_device == PAGE0->mem_boot.pz_dp.dp_bc[3] &&
		    paa->pa_function == PAGE0->mem_boot.pz_dp.dp_bc[4]) {
			/*
			 * This is the PCI - PCI bridge in front of the boot
			 * device.
			 */
			boot_device = dev;
		}
	}
	if (device_is_a(pdev, "pci") && boot_device == device_parent(pdev)) {
		struct pci_attach_args *paa = (struct pci_attach_args*)aux;

		if (paa->pa_device == PAGE0->mem_boot.pz_dp.dp_bc[5] &&
		    paa->pa_function == PAGE0->mem_boot.pz_dp.dp_mod) {
			/*
			 * This is (the controller of) the boot device.
			 */
			boot_device = dev;
		}
	}
	/*
	 * When SCSI devices are attached, we look if the SCSI device hangs
	 * below the controller remembered in boot_device. If so, we compare
	 * the SCSI ID and LUN with the DP layer information. If they match
	 * we found the boot device.
	 */
	if (device_is_a(pdev, "scsibus") &&
	    boot_device == device_parent(pdev)) {
		struct scsipibus_attach_args *saa = aux;
		struct scsipi_periph *p = saa->sa_periph;
		
		if (p->periph_target == PAGE0->mem_boot.pz_dp.dp_layers[0] &&
		    p->periph_lun == PAGE0->mem_boot.pz_dp.dp_layers[1]) {
			/* This is the boot device. */
			boot_device = dev;
		}
	}

	hppa_pagezero_unmap(pagezero_cookie);
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

	pagezero_cookie = hppa_pagezero_map();
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

	hppa_pagezero_unmap(pagezero_cookie);
#endif /* DEBUG */

	if (boot_device != NULL)
		printf("boot device: %s\n", device_xname(boot_device));
	booted_device = boot_device;
	rootconf();
}

void
hppa_walkbus(struct confargs *ca)
{
	struct hppa_pdcmodule nhm, *hm;
	int i;

	if (ca->ca_hpabase == 0)
		return;
	
	aprint_debug(">> Walking bus at HPA 0x%lx\n", ca->ca_hpabase);

	for (i = 0; i < ca->ca_nmodules; i++) {
		int error;

 		memset(&nhm, 0, sizeof(nhm));
		nhm.hm_dp.dp_bc[0] = ca->ca_dp.dp_bc[1];
		nhm.hm_dp.dp_bc[1] = ca->ca_dp.dp_bc[2];
		nhm.hm_dp.dp_bc[2] = ca->ca_dp.dp_bc[3];
		nhm.hm_dp.dp_bc[3] = ca->ca_dp.dp_bc[4];
		nhm.hm_dp.dp_bc[4] = ca->ca_dp.dp_bc[5];
		nhm.hm_dp.dp_bc[5] = ca->ca_dp.dp_mod;
		nhm.hm_hpa = ca->ca_hpabase + IOMOD_HPASIZE * i;
		nhm.hm_hpasz = 0;
		nhm.hm_dp.dp_mod = i;
		nhm.hm_naddrs = 0;

		error = pdcproc_iodc_read(nhm.hm_hpa, IODC_DATA, NULL,
		    &nhm.hm_pir, sizeof(nhm.hm_pir), &nhm.hm_type,
		    sizeof(nhm.hm_type));
		if (error < 0)
			continue;

		aprint_debug(">> HPA 0x%lx[0x%x]", nhm.hm_hpa,
		    nhm.hm_hpasz);

		TAILQ_FOREACH(hm, &hppa_pdcmodule_list, hm_link) {
			if (nhm.hm_hpa == hm->hm_hpa) {
				aprint_debug(" found by firmware\n");
				break;
			}
		}

		/* If we've found the module move onto the next one. */
		if (hm)
			continue;

		/* Expect PDC to report devices of the following types */
		if (nhm.hm_type.iodc_type == HPPA_TYPE_FIO) {
			aprint_debug(" expected to be missing\n");
			continue;
		}

		hppa_pdcmodule_create(&nhm, "Bus walk");
	}
}

void
pdc_scanbus(device_t self, struct confargs *ca,
    device_t (*callback)(device_t, struct confargs *))
{
	struct hppa_pdcmodule *hm;
	struct confargs nca;
	device_t dev;
	int ia;

	hppa_walkbus(ca);

	TAILQ_FOREACH(hm, &hppa_pdcmodule_list, hm_link) {
		char buf[128];
		int error;

		if (hm->hm_registered)
			continue;

		if (!(hm->hm_dp.dp_bc[0] == ca->ca_dp.dp_bc[1] &&
		    hm->hm_dp.dp_bc[1] == ca->ca_dp.dp_bc[2] &&
		    hm->hm_dp.dp_bc[2] == ca->ca_dp.dp_bc[3] &&
		    hm->hm_dp.dp_bc[3] == ca->ca_dp.dp_bc[4] &&
		    hm->hm_dp.dp_bc[4] == ca->ca_dp.dp_bc[5] &&
		    hm->hm_dp.dp_bc[5] == ca->ca_dp.dp_mod))
			continue;

		memset(&nca, 0, sizeof(nca));
		nca.ca_iot = ca->ca_iot;
		nca.ca_dmatag = ca->ca_dmatag;
		nca.ca_pir = hm->hm_pir;
		nca.ca_type = hm->hm_type;
		nca.ca_hpa = hm->hm_hpa;
		nca.ca_dp = hm->hm_dp;
		nca.ca_hpa = hm->hm_hpa;
		nca.ca_hpasz = hm->hm_hpasz;

		if (hm->hm_naddrs) {
			if (hm->hm_naddrs > HPPA_MAXIOADDRS) {
				nca.ca_naddrs = HPPA_MAXIOADDRS;
				aprint_error("WARNING: too many (%d) addrs\n",
				    hm->hm_naddrs);
			} else
				nca.ca_naddrs = hm->hm_naddrs;

			aprint_debug(">> ADDRS[%d/%d]: ", nca.ca_naddrs,
			    hm->hm_modindex);

			KASSERT(hm->hm_modindex != -1);
			for (ia = 0; ia < nca.ca_naddrs; ia++) {
				struct pdc_system_map_find_addr pdc_find_addr;

				error = pdcproc_system_map_find_addr(
				    &pdc_find_addr, hm->hm_modindex, ia + 1);
				if (error < 0)
					break;
				nca.ca_addrs[ia].addr = pdc_find_addr.hpa;
				nca.ca_addrs[ia].size =
				    pdc_find_addr.size << PGSHIFT;

				aprint_debug(" 0x%lx[0x%x]",
				    nca.ca_addrs[ia].addr,
				    nca.ca_addrs[ia].size);
			}
			aprint_debug("\n");
		}

		aprint_debug(">> HPA 0x%lx[0x%x]\n", nca.ca_hpa,
		    nca.ca_hpasz);

		snprintb(buf, sizeof(buf), PZF_BITS, nca.ca_dp.dp_flags);
		aprint_debug(">> probing: flags %s ", buf);
		if (nca.ca_dp.dp_mod >=0) {
			int n;

			aprint_debug(" path ");
			for (n = 0; n < 6; n++) {
				if (nca.ca_dp.dp_bc[n] >= 0)
					aprint_debug("%d/",
					    nca.ca_dp.dp_bc[n]);
			}
			aprint_debug("%d", nca.ca_dp.dp_mod);
		}

		aprint_debug(" type %x sv %x\n",
		    nca.ca_type.iodc_type, nca.ca_type.iodc_sv_model);

		nca.ca_irq = HPPACF_IRQ_UNDEF;
		nca.ca_name = hppa_mod_info(nca.ca_type.iodc_type,
		    nca.ca_type.iodc_sv_model);

		dev = callback(self, &nca);

		if (dev)
			hm->hm_registered = true;
		
	}
}

static const struct hppa_mod_info hppa_knownmods[] = {
#include <hppa/dev/cpudevs_data.h>
};

const char *
hppa_mod_info(int type, int sv)
{
	const struct hppa_mod_info *mi;
	static char fakeid[32];
	int i;

	for (i = 0, mi = hppa_knownmods; i < __arraycount(hppa_knownmods);
	    i++, mi++) {
		if (mi->mi_type == type && mi->mi_sv == sv) {
			break;
		}
	}

	if (i == __arraycount(hppa_knownmods)) {
		snprintf(fakeid, sizeof(fakeid), "type %x, sv %x", type, sv);
		return fakeid;
	}

	return mi->mi_name;
}

/*
 * Create the device on our device list.  Keep the devices in order. */
struct hppa_pdcmodule *
hppa_pdcmodule_create(struct hppa_pdcmodule *hm, const char *who)
{
	struct hppa_pdcmodule *nhm, *ahm;
	int i;
	
	nhm = kmem_zalloc(sizeof(*nhm), KM_SLEEP);

	nhm->hm_registered = false;
	nhm->hm_pir = hm->hm_pir;
	nhm->hm_type = hm->hm_type;
	nhm->hm_dp = hm->hm_dp;
	nhm->hm_hpa = hm->hm_hpa;
	nhm->hm_hpasz = hm->hm_hpasz;
	nhm->hm_naddrs = hm->hm_naddrs;
	nhm->hm_modindex = hm->hm_modindex;

	/* Find start of new path */
	for (i = 0; i < 6; i++) {
		if (hm->hm_dp.dp_bc[i] != -1)
			break;
	}

	/*
	 * Look, in reverse, for the first device that has a path before our
	 * new one.  In reverse because PDC reports most (all?) devices in path
	 * order and therefore the common case is to add to the end of the
	 * list.
	 */
	TAILQ_FOREACH_REVERSE(ahm, &hppa_pdcmodule_list, hppa_pdcmodule_head,
	    hm_link) {
		int check;
		int j, k;
		
		for (j = 0; j < 6; j++) {
			if (ahm->hm_dp.dp_bc[j] != -1)
				break;
		}

		for (check = 0, k = i; j < 7 && k < 7; j++, k++) {
			char nid, aid;

			nid = (k == 6) ? hm->hm_dp.dp_mod : hm->hm_dp.dp_bc[k];
			aid = (j == 6) ? ahm->hm_dp.dp_mod : ahm->hm_dp.dp_bc[j];

			if (nid == aid)
				continue;
			check = nid - aid;
			break;
		}
		if (check >= 0)
			break;
		else if (check < 0)
			continue;
	}
	if (ahm == NULL)
		TAILQ_INSERT_HEAD(&hppa_pdcmodule_list, nhm, hm_link);
	else
		TAILQ_INSERT_AFTER(&hppa_pdcmodule_list, ahm, nhm, hm_link);

	if (hm->hm_dp.dp_mod >= 0) {
		int n;

		aprint_debug(">> %s device at path ", who);
		for (n = 0; n < 6; n++) {
			if (hm->hm_dp.dp_bc[n] >= 0)
				aprint_debug("%d/", hm->hm_dp.dp_bc[n]);
		}
		aprint_debug("%d addrs %d\n", hm->hm_dp.dp_mod,
		    hm->hm_naddrs);
	}

	return nhm;
}

/*
 * This is used for Snake machines
 */
static struct hppa_pdcmodule *
hppa_memmap_query(struct device_path *devp)
{
	static struct hppa_pdcmodule nhm;
	struct pdc_memmap pdc_memmap;
	int error;

	error = pdcproc_memmap(&pdc_memmap, devp);
	
	if (error < 0)
		return NULL;

	memset(&nhm, 0, sizeof(nhm));
	nhm.hm_dp = *devp;
	nhm.hm_hpa = pdc_memmap.hpa;
	nhm.hm_hpasz = pdc_memmap.morepages;
	nhm.hm_naddrs = 0;
	nhm.hm_modindex = -1;

	error = pdcproc_iodc_read(nhm.hm_hpa, IODC_DATA, NULL, &nhm.hm_pir,
	    sizeof(nhm.hm_pir), &nhm.hm_type, sizeof(nhm.hm_type));

	if (error < 0)
		return NULL;

	return hppa_pdcmodule_create(&nhm, "PDC (memmap)");
}


static void
hppa_pdc_snake_scan(void)
{
	struct device_path path;
	struct hppa_pdcmodule *hm;
	int im, ba;

	memset(&path, 0, sizeof(path));
	for (im = 0; im < 16; im++) {
		path.dp_bc[0] = path.dp_bc[1] = path.dp_bc[2] =
		path.dp_bc[3] = path.dp_bc[4] = path.dp_bc[5] = -1;
		path.dp_mod = im;

		hm = hppa_memmap_query(&path);

		if (!hm)
			continue;

		if (hm->hm_type.iodc_type != HPPA_TYPE_BHA)
			continue;

		path.dp_bc[0] = path.dp_bc[1] =
		path.dp_bc[2] = path.dp_bc[3] = -1;
		path.dp_bc[4] = im;
		path.dp_bc[5] = 0;

		for (ba = 0; ba < 16; ba++) {
			path.dp_mod = ba;
			hppa_memmap_query(&path);
		}
	}
}

static void
hppa_pdc_system_map_scan(void)
{
	struct pdc_system_map_find_mod pdc_find_mod;
	struct device_path path;
	struct hppa_pdcmodule hm;
	int error;
	int im;

	for (im = 0; im < HPPA_SYSTEMMAPMODULES; im++) {
		memset(&path, 0, sizeof(path));
		error = pdcproc_system_map_find_mod(&pdc_find_mod, &path, im);
		if (error == PDC_ERR_NMOD)
			break;

		if (error < 0)
			continue;

		memset(&hm, 0, sizeof(hm));
		hm.hm_dp = path;
		hm.hm_hpa = pdc_find_mod.hpa;
		hm.hm_hpasz = pdc_find_mod.size << PGSHIFT;
		hm.hm_naddrs = pdc_find_mod.naddrs;
		hm.hm_modindex = im;

		error = pdcproc_iodc_read(hm.hm_hpa, IODC_DATA, NULL,
		    &hm.hm_pir, sizeof(hm.hm_pir), &hm.hm_type,
		    sizeof(hm.hm_type));
		if (error < 0)
			continue;

		hppa_pdcmodule_create(&hm, "PDC (system map)");
	}
}

void
hppa_modules_scan(void)
{
	switch (pdc_gettype()) {
	case PDC_TYPE_SNAKE:
		hppa_pdc_snake_scan();
		break;

	case PDC_TYPE_UNKNOWN:
		hppa_pdc_system_map_scan();
	}
}

void
hppa_modules_done(void)
{
	struct hppa_pdcmodule *hm, *nhm;

	TAILQ_FOREACH_SAFE(hm, &hppa_pdcmodule_list, hm_link, nhm) {
		TAILQ_REMOVE(&hppa_pdcmodule_list, hm, hm_link);
		kmem_free(hm, sizeof(*hm));
	}
}
