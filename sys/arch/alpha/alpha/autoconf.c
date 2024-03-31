/* $NetBSD: autoconf.c,v 1.59 2024/03/31 19:06:30 thorpej Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.59 2024/03/31 19:06:30 thorpej Exp $");

#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <dev/cons.h>

#include <dev/pci/pcivar.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/autoconf.h>
#include <machine/alpha.h>
#include <machine/cpu.h>
#include <machine/prom.h>
#include <machine/cpuconf.h>
#include <machine/intr.h>

struct bootdev_data	*bootdev_data;

static void	parse_prom_bootdev(void);

/*
 * cpu_configure:
 * called at boot time, configure all devices on system
 */
void
cpu_configure(void)
{

	parse_prom_bootdev();

	/*
	 * Disable interrupts during autoconfiguration.  splhigh() won't
	 * work, because it simply _raises_ the IPL, so if machine checks
	 * are disabled, they'll stay disabled.  Machine checks are needed
	 * during autoconfig.
	 */
	(void)alpha_pal_swpipl(ALPHA_PSL_IPL_HIGH);
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");
	(void)spl0();

	/*
	 * Note that bootstrapping is finished, and set the HWRPB up
	 * to do restarts.
	 */
	hwrpb_restart_setup();
}

static void
qemu_find_rootdev(void)
{
	char buf[32] = { 0 };

	/*
	 * Check for "rootdev=wd0".
	 */
	if (! prom_qemu_getenv("rootdev", buf, sizeof(buf))) {
		/*
		 * Check "root=/dev/wd0a", "root=/dev/hda1", etc.
		 */
		if (! prom_qemu_getenv("root", buf, sizeof(buf))) {
			printf("WARNING: no rootdev= or root= arguments "
			       "provided by Qemu\n");
			return;
		}
	}

	const size_t devlen = strlen("/dev/");
	const char *cp = buf;
	char *ecp = &buf[sizeof(buf) - 1];

	/* Find the start of the device xname. */
	if (strlen(cp) > devlen && strncmp(cp, "/dev/", devlen) == 0) {
		cp += devlen;
	}

	/* Now strip any partition letter off the end. */
	while (ecp != cp) {
		if (*ecp >= '0' && *ecp <= '9') {
			break;
		}
		*ecp-- = '\0';
	}

	snprintf(bootinfo.booted_dev, sizeof(bootinfo.booted_dev),
	    "root=%s", cp);
	booted_device = device_find_by_xname(cp);
}

static bool
parse_dec_macaddr(const char *str, uint8_t enaddr[ETHER_ADDR_LEN])
{
	char *cp;
	long long l;
	int i;

	/*
	 * DEC Ethernet address strings are formatted like so:
	 *
	 *	XX-XX-XX-XX-XX-XX
	 */

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		l = strtoll(str, &cp, 16);
		if (l < 0 || l > 0xff) {
			/* Not a valid MAC address. */
			return false;
		}
		if (*cp == '-') {
			/* Octet separator. */
			enaddr[i] = (uint8_t)l;
			str = cp + 1;
			continue;
		}
		if (*cp == ' ' || *cp == '\0') {
			/* End of the string. */
			enaddr[i] = (uint8_t)l;
			return i == ETHER_ADDR_LEN - 1;
		}
		/* Bogus character. */
		break;
	}

	/* Encountered bogus character or didn't reach end of string. */
	return false;
}

static void
netboot_find_rootdev_planb(void)
{
	struct psref psref;
	uint8_t enaddr[ETHER_ADDR_LEN];
	char ifname[IFNAMSIZ];
	int i;

	if (strncasecmp(bootinfo.booted_dev, "BOOTP ", 6) != 0 &&
	    strncasecmp(bootinfo.booted_dev, "MOP ", 4) != 0) {
		/* We weren't netbooted. */
		return;
	}

	for (i = 2; bootinfo.booted_dev[i] != '\0'; i++) {
		if (bootinfo.booted_dev[i] == '-') {
			if (parse_dec_macaddr(&bootinfo.booted_dev[i - 2],
					      enaddr)) {
				/* Found it! */
				break;
			}
		}
	}
	if (bootinfo.booted_dev[i] == '\0') {
		/* No MAC address in string. */
		return;
	}

	/* Now try to look up the interface by the link address. */
	struct ifnet *ifp = if_get_bylla(enaddr, ETHER_ADDR_LEN, &psref);
	if (ifp == NULL) {
		/* No interface attached with that MAC address. */
		return;
	}

	strlcpy(ifname, if_name(ifp), sizeof(ifname));
	if_put(ifp, &psref);

	/* Ok! Now look up the device_t by name! */
	booted_device = device_find_by_xname(ifname);
}

void
cpu_rootconf(void)
{

	if (booted_device == NULL && alpha_is_qemu) {
		qemu_find_rootdev();
	}

	if (booted_device == NULL) {
		/*
		 * It's possible that we netbooted from an Ethernet
		 * interface that can't be matched via the usual
		 * logic in device_register() (a DE204 in an ISA slot,
		 * for example).  In these cases, the console may have
		 * provided us with a MAC address that we can use to
		 * try and find the interface, * e.g.:
		 *
		 *	BOOTP 1 1 0 0 0 5 0 08-00-2B-xx-xx-xx 1
		 */
		netboot_find_rootdev_planb();
	}

	if (booted_device == NULL) {
		printf("WARNING: can't figure what device matches \"%s\"\n",
		    bootinfo.booted_dev);
	}
	rootconf();
}

static inline int
atoi(const char *s)
{
	return (int)strtoll(s, NULL, 10);
}

static void
parse_prom_bootdev(void)
{
	static char hacked_boot_dev[128];
	static struct bootdev_data bd;
	char *cp, *scp, *boot_fields[8];
	int i, done;

	booted_device = NULL;
	booted_partition = 0;
	bootdev_data = NULL;

	memcpy(hacked_boot_dev, bootinfo.booted_dev,
	    uimin(sizeof bootinfo.booted_dev, sizeof hacked_boot_dev));
#if 0
	printf("parse_prom_bootdev: boot dev = \"%s\"\n", hacked_boot_dev);
#endif

	i = 0;
	scp = cp = hacked_boot_dev;
	for (done = 0; !done; cp++) {
		if (*cp != ' ' && *cp != '\0')
			continue;
		if (*cp == '\0')
			done = 1;

		*cp = '\0';
		boot_fields[i++] = scp;
		scp = cp + 1;
		if (i == 8)
			done = 1;
	}
	if (i != 8)
		return;		/* doesn't look like anything we know! */

#if 0
	printf("i = %d, done = %d\n", i, done);
	for (i--; i >= 0; i--)
		printf("%d = %s\n", i, boot_fields[i]);
#endif

	bd.protocol = boot_fields[0];
	bd.bus = atoi(boot_fields[1]);
	bd.slot = atoi(boot_fields[2]);
	bd.channel = atoi(boot_fields[3]);
	bd.remote_address = boot_fields[4];
	bd.unit = atoi(boot_fields[5]);
	bd.boot_dev_type = atoi(boot_fields[6]);
	bd.ctrl_dev_type = boot_fields[7];

#if 0
	printf("parsed: proto = %s, bus = %d, slot = %d, channel = %d,\n",
	    bd.protocol, bd.bus, bd.slot, bd.channel);
	printf("\tremote = %s, unit = %d, dev_type = %d, ctrl_type = %s\n",
	    bd.remote_address, bd.unit, bd.boot_dev_type, bd.ctrl_dev_type);
#endif

	bootdev_data = &bd;
}

void
device_register(device_t dev, void *aux)
{
#if NPCI > 0
	device_t parent = device_parent(dev);

	if (parent != NULL && device_is_a(parent, "pci"))
		device_pci_register(dev, aux);
#endif
	if (platform.device_register)
		(*platform.device_register)(dev, aux);
}
