/*	$NetBSD: autoconf.c,v 1.15.38.1 2007/06/21 18:49:45 garbled Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.15.38.1 2007/06/21 18:49:45 garbled Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/powerpc.h>
#include <machine/stdarg.h>

#include <dev/ofw/openfirm.h>
#include <dev/pci/pcivar.h>
#include <dev/ofw/ofw_pci.h>

static void canonicalize_bootpath(void);
void ofw_stack(void);
void ofbcopy(const void*, void *, size_t);

extern char bootpath[];
char cbootpath[256];
int console_node = 0;
int console_instance = 0;

/*
 * Determine device configuration for a machine.
 */
void
cpu_configure()
{

	canonicalize_bootpath();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	(void)spl0();
}

/*
 * Setup root device.
 * Configure swap area.
 */
void
cpu_rootconf()
{

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

void
device_register(struct device *dev, void *aux)
{
	return; /* XXX */
}

static void
canonicalize_bootpath()
{
	int node;
	char *p;
	char last[32];

	printf("bootpath %s\n", bootpath);
	/*
	 * Strip kernel name.  bootpath contains "OF-path"/"kernel".
	 *
	 * for example:
	 *   /pci/scsi/disk@0:0/netbsd		(FirePower)
	 */
	strcpy(cbootpath, bootpath);
	while ((node = OF_finddevice(cbootpath)) == -1) {
		if ((p = strrchr(cbootpath, '/')) == NULL)
			break;
		*p = 0;
	}

	if (node == -1) {
		/* Cannot canonicalize... use bootpath anyway. */
		strcpy(cbootpath, bootpath);

		return;
	}

	/*
	 * cbootpath is a valid OF path.  Use package-to-path to
	 * canonicalize pathname.
	 */

	/* Back up the last component for later use. */
	if ((p = strrchr(cbootpath, '/')) != NULL)
		strcpy(last, p + 1);
	else
		last[0] = 0;

	memset(cbootpath, 0, sizeof(cbootpath));
	OF_package_to_path(node, cbootpath, sizeof(cbootpath) - 1);

	/*
	 * At this point, cbootpath contains like:
	 * "/pci/scsi@1/disk"
	 *
	 * The last component may have no address... so append it.
	 */
	p = strrchr(cbootpath, '/');
	if (p != NULL && strchr(p, '@') == NULL) {
		/* Append it. */
		if ((p = strrchr(last, '@')) != NULL)
			strcat(cbootpath, p);
	}

	/*
	 * Strip off the partition again (saving it in booted_parition).
	 */
	if ((p = strrchr(cbootpath, ':')) != NULL) {
		*p++ = 0;
		booted_partition = *p - 'a';
	}
	printf("cbootpath %s\n", cbootpath);
}

/*
 * Find OF-device corresponding to the PCI device.
 */
int
pcidev_to_ofdev(pci_chipset_tag_t pc, pcitag_t tag)
{
	int bus, dev, func;
	struct ofw_pci_register reg;
	int p, q, l, b, d, f;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	for (q = OF_peer(0); q; q = p) {
		l = OF_getprop(q, "assigned-addresses", &reg, sizeof(reg));
		if (l == sizeof(reg)) {
			b = OFW_PCI_PHYS_HI_BUS(reg.phys_hi);
			d = OFW_PCI_PHYS_HI_DEVICE(reg.phys_hi);
			f = OFW_PCI_PHYS_HI_FUNCTION(reg.phys_hi);

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

int
OF_interpret(const char *cmd, int nargs, int nreturns, ...)
{
	va_list ap;
	int i, len, status;
	static struct {
		const char *name;
		uint32_t nargs;
		uint32_t nreturns;
		uint32_t slots[16];
	} args = {
		"interpret",
		1,
		2,
	};

	ofw_stack();
	if (nreturns > 8)
		return -1;
	if ((len = strlen(cmd)) >= PAGE_SIZE)
		return -1;
	ofbcopy(cmd, OF_buf, len + 1);
	i = 0;
	args.slots[i] = (uint32_t)OF_buf;
	args.nargs = nargs + 1;
	args.nreturns = nreturns + 1;
	va_start(ap, nreturns);
	i++;
	while (i < args.nargs) {
		args.slots[i] = (uint32_t)va_arg(ap, uint32_t *);
		i++;
	}

	if (openfirmware(&args) == -1)
		return -1;
	status = args.slots[i];
	i++;

	while (i < args.nargs + args.nreturns) {
		*va_arg(ap, uint32_t *) = args.slots[i];
		i++;
	}
	va_end(ap);
	return status;
}
