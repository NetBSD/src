/*	$NetBSD: autoconf.c,v 1.7 1998/12/10 20:49:17 tsubai Exp $	*/

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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/stdarg.h>

#include <dev/ofw/openfirm.h>
#include <dev/pci/pcivar.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

extern int cold;

void findroot __P((void));

struct device *booted_device;	/* boot device */
int booted_partition;		/* ...and partition on that device */

#define INT_ENABLE_REG (interrupt_reg + 0x24)
#define INT_CLEAR_REG  (interrupt_reg + 0x28)
extern u_char *interrupt_reg;

/*
 * Determine device configuration for a machine.
 */
void
configure()
{
	interrupt_reg = mapiodev(0xf3000000, NBPG);
	out32rb(INT_ENABLE_REG, 0);		/* disable all intr. */
	out32rb(INT_CLEAR_REG, 0xffffffff);	/* clear pending intr. */

	calc_delayconst();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	(void)spl0();
	cold = 0;
}

/*
 * Setup root device.
 * Configure swap area.
 */
void
cpu_rootconf()
{
	findroot();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition, dev_name2blk);
}

/*
 * Try to find the device we were booted from to set rootdev.
 */
void
findroot()
{
	int chosen, node, pnode;
	u_int targ, lun = 0;	/* XXX lun */
	struct device *dv;
	char *controller, *addr, p[64];

	booted_device = NULL;

	if ((chosen = OF_finddevice("/chosen")) == -1)
		goto out;
	bzero(p, sizeof(p));
	if (OF_getprop(chosen, "bootpath", p, sizeof(p)) == -1)
		goto out;
	if ((addr = strrchr(p, '@')) == NULL)	/* XXX fd:0 case... */
		goto out;
	targ = addr[1] - '0';
	booted_partition = 0;	/* booted_partition = addr[3] - '0'; */

	if ((node = OF_finddevice(p)) == -1)
		goto out;
	if ((pnode = OF_parent(node)) == -1)
		goto out;
	bzero(p, sizeof(p));
	if (OF_getprop(pnode, "name", p, sizeof(p)) == -1)
		goto out;

	controller = "";
	if (strcmp(p, "53c94") == 0) controller = "esp";
	if (strcmp(p, "mesh") == 0) controller = "mesh";
	if (strcmp(p, "ide") == 0) controller = "wdc";
	if (strcmp(p, "ata") == 0) controller = "wdc";
	if (strcmp(p, "ATA") == 0) controller = "wdc";

	for (dv = alldevs.tqh_first; dv; dv=dv->dv_list.tqe_next) {
		if (dv->dv_class != DV_DISK)
			continue;

		if (strncmp(dv->dv_xname, "sd", 2) == 0) {
			struct scsibus_softc *sdv = (void *)dv->dv_parent;

			/* sd? at scsibus at esp/mesh */
			if (strncmp(dv->dv_parent->dv_parent->dv_xname,
				    controller, strlen(controller)) != 0)
				continue;
			if (targ > 7 || lun > 7)
				goto out;
			if (sdv->sc_link[targ][lun] == NULL)
				continue;
			booted_device = dv;
			break;
		}

		if (strncmp(dv->dv_xname, "wd", 2) == 0) {
			struct wdc_softc *wdv = (void *)dv->dv_parent;

			if (strncmp(dv->dv_parent->dv_xname,
				    controller, strlen(controller)) != 0)
				continue;
			if (targ >= wdv->nchannels
			 || wdv->channels == NULL
			 || wdv->channels[targ] == NULL)
				continue;
			booted_device = dv;
			break;
		}
	}

out:
	dk_cleanup();
}

int
#ifdef __STDC__
OF_interpret(char *cmd, int nreturns, ...)
#else
OF_interpret(cmd, nreturns, va_alist)
	char *cmd;
	int nreturns;
	va_dcl
#endif
{
	va_list ap;
	int i;
	static struct {
		char *name;
		int nargs;
		int nreturns;
		char *cmd;
		int status;
		int results[8];
	} args = {
		"interpret",
		1,
		2,
	};

	ofw_stack();
	if (nreturns > 8)
		return -1;
	if ((i = strlen(cmd)) >= NBPG)
		return -1;
	ofbcopy(cmd, OF_buf, i + 1);
	args.cmd = OF_buf;
	args.nargs = 1;
	args.nreturns = nreturns + 1;
	if (openfirmware(&args) == -1)
		return -1;
	va_start(ap, nreturns);
	for (i = 0; i < nreturns; i++)
		*va_arg(ap, int *) = args.results[i];
	va_end(ap);
	return args.status;
}

/*
 * Find OF-device corresponding to the PCI device.
 */
int
pcidev_to_ofdev(pa)
	struct pci_attach_args *pa;
{
	int bus, dev, func;
	u_int reg[5];
	int p, q;
	int l, b, d, f;

	pci_decompose_tag(pa->pa_pc, pa->pa_tag, &bus, &dev, &func);

	for (q = OF_peer(0); q; q = p) {
		l = OF_getprop(q, "assigned-addresses", reg, sizeof(reg));
		if (l > 4) {
			b = (reg[0] >> 16) & 0xff;
			d = (reg[0] >> 11) & 0x1f;
			f = (reg[0] >> 8) & 0x07;

			if (b == bus && d == dev && f == func)
				return q;
		}
		if ((p = OF_child(q)))
			continue;
		while (q) {
			if ((p = OF_peer(q)))
				break;
			q = OF_parent(q);
		}
	}
	return 0;
}
