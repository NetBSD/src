/*	$NetBSD: autoconf.c,v 1.8 2003/07/15 02:29:26 lukem Exp $	*/

/*	$OpenBSD: autoconf.c,v 1.15 2001/06/25 00:43:10 mickey Exp $	*/

/*
 * Copyright (c) 1998-2001 Michael Shalayeff
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.8 2003/07/15 02:29:26 lukem Exp $");

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

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/cons.h>

#include <hp700/hp700/machdep.h>
#include <hp700/hp700/intr.h>
#include <hp700/dev/cpudevs.h>

void (*cold_hook) __P((void)); /* see below */
register_t	kpsw = PSW_Q | PSW_P | PSW_C | PSW_D;

/*
 * LED blinking thing
 */
#ifdef USELEDS
int _hp700_led_on_cycles[_HP700_LEDS_BLINKABLE];
static struct callout hp700_led_callout;
static void hp700_led_blinker __P((void *));
extern int hz;
#endif

/* XXX added for RAIDframe but so far ignored by us. */
struct device *booted_device;

/*
 * cpu_configure:
 * called at boot time, configure all devices on system
 */
void
cpu_configure()
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
	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	/* in spl*() we trust */
	hp700_intr_init();
	__asm __volatile("ssm %0, %%r0" :: "i" (PSW_I));
	kpsw |= PSW_I;
	spl0();

	cold = 0;
	if (cold_hook)
		(*cold_hook)();

#ifdef USELEDS
	memset(_hp700_led_on_cycles, 0, sizeof(_hp700_led_on_cycles));
	callout_init(&hp700_led_callout);
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
		register int b;
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
hp700_led_blinker(arg)
	void *arg;
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
cpu_dumpconf()
{
	const struct bdevsw *bdev;
	extern int dumpsize;
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
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

/* This takes the args: name, ctlr, unit */
typedef struct device * (*findfunc_t) __P((char *, int, int));

static struct device * find_dev_byname __P((char *));
static struct device * net_find  __P((char *, int, int));
static struct device * scsi_find __P((char *, int, int));

struct prom_n2f {
	const char name[4];
	findfunc_t func;
};
static struct prom_n2f prom_dev_table[] = {
	{ "ie",		net_find },
	{ "sd",		scsi_find },
	{ "st",		scsi_find },
	{ "cd",		scsi_find },
	{ "",		0 },
};

/*
 * Choose root and swap devices.
 */
void
cpu_rootconf()
{
	struct prom_n2f *nf;
	struct device *boot_device;
	int boot_partition;
	char *devname;
	findfunc_t find;
	char promname[4];
	char partname[4];
	int prom_ctlr, prom_unit, prom_part;

	/* Get the PROM boot path and take it apart. */
	/* XXX fredette - need something real here: */
	strcpy(promname, "ie");
	prom_ctlr = prom_unit = prom_part = 0;

	/* Default to "unknown" */
	boot_device = NULL;
	boot_partition = 0;
	devname = "<unknown>";
	partname[0] = '\0';
	find = NULL;

	/* Do we know anything about the PROM boot device? */
	for (nf = prom_dev_table; nf->func; nf++)
		if (!strcmp(nf->name, promname)) {
			find = nf->func;
			break;
		}
	if (find)
		boot_device = (*find)(promname, prom_ctlr, prom_unit);
	if (boot_device) {
		devname = boot_device->dv_xname;
		if (boot_device->dv_class == DV_DISK) {
			boot_partition = prom_part & 7;
			partname[0] = 'a' + boot_partition;
			partname[1] = '\0';
		}
	}

	printf("boot device: %s%s\n", devname, partname);
	setroot(boot_device, boot_partition);
}

/*
 * Functions to find devices using PROM boot parameters.
 */

/*
 * Network device:  Just use controller number.
 */
static struct device *
net_find(name, ctlr, unit)
	char *name;
	int ctlr, unit;
{
	char tname[16];

	sprintf(tname, "%s%d", name, ctlr);
	return (find_dev_byname(tname));
}

/*
 * SCSI device:  The controller number corresponds to the
 * scsibus number, and the unit number is (targ*8 + LUN).
 */
static struct device *
scsi_find(name, ctlr, unit)
	char *name;
	int ctlr, unit;
{
	struct device *scsibus;
	struct scsibus_softc *sbsc;
	struct scsipi_periph *periph;
	int target, lun;
	char tname[16];

	sprintf(tname, "scsibus%d", ctlr);
	scsibus = find_dev_byname(tname);
	if (scsibus == NULL)
		return (NULL);

	/* Compute SCSI target/LUN from PROM unit. */
	target = (unit >> 3) & 7;
	lun = unit & 7;

	/* Find the device at this target/LUN */
	sbsc = (struct scsibus_softc *)scsibus;
	periph = scsipi_lookup_periph(sbsc->sc_channel, target, lun);
	if (periph == NULL)
		return (NULL);

	return (periph->periph_dev);
}

/*
 * Given a device name, find its struct device
 * XXX - Move this to some common file?
 */
static struct device *
find_dev_byname(name)
	char *name;
{
	struct device *dv;

	for (dv = alldevs.tqh_first; dv != NULL;
	    dv = dv->dv_list.tqe_next) {
		if (!strcmp(dv->dv_xname, name)) {
			return(dv);
		}
	}
	return (NULL);
}

#ifdef RAMDISK_HOOKS
/*static struct device fakerdrootdev = { DV_DISK, {}, NULL, 0, "rd0", NULL };*/
#endif

void
pdc_scanbus(self, bus, maxmod, callback)
	struct device *self;
	int bus, maxmod;
	void (*callback) __P((struct device *, struct confargs *));
{
	struct pdc_memmap pdc_memmap;
	struct device_path dp;
	register int i;

	for (i = maxmod; i--; ) {
		struct confargs nca;
		struct pdc_iodc_read pdc_iodc_read;

		dp.dp_bc[0] = dp.dp_bc[1] = dp.dp_bc[2] = dp.dp_bc[3] = -1;
		dp.dp_bc[4] = bus;
		dp.dp_bc[5] = bus < 0? -1 : 0;
		dp.dp_mod = i;

		if (pdc_call((iodcio_t)pdc, 0, PDC_MEMMAP,
			     PDC_MEMMAP_HPA, &pdc_memmap, &dp) < 0)
			continue;

		if (pdc_call((iodcio_t)pdc, 0, PDC_IODC, PDC_IODC_READ,
			     &pdc_iodc_read, pdc_memmap.hpa, IODC_DATA,
			     &nca.ca_type, sizeof(nca.ca_type)) < 0)
			continue;

		nca.ca_mod = i;
		nca.ca_hpa = pdc_memmap.hpa;
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
hppa_mod_info(type, sv)
	int type, sv;
{
	register const struct hppa_mod_info *mi;
	static char fakeid[32];

	for (mi = hppa_knownmods; mi->mi_type >= 0 &&
	     (mi->mi_type != type || mi->mi_sv != sv); mi++);

	if (mi->mi_type < 0) {
		sprintf(fakeid, "type %x, sv %x", type, sv);
		return fakeid;
	} else
		return mi->mi_name;
}

