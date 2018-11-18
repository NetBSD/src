/*	$NetBSD: apbus_subr.c,v 1.9.22.1 2018/11/18 11:54:02 martin Exp $	*/

/*-
 * Copyright (C) 1999 SHIMIZU Ryo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apbus_subr.c,v 1.9.22.1 2018/11/18 11:54:02 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <mips/pte.h>

#include <machine/wired_map.h>

#include <newsmips/apbus/apbusvar.h>

static void apctl_dump(struct apbus_ctl *);

#define	APBUS_ROMWORK_VA	0xfff00000

void
apbus_map_romwork(void)
{
	static bool mapped = false;
	vaddr_t apbd_work_va;
	vsize_t apbd_work_sz;
	paddr_t apbd_work_pa;

	if (!mapped) {
		/* map PROM work RAM into VA 0xFFF00000 - 0xFFFFFFFF */
		apbd_work_va = APBUS_ROMWORK_VA;
		apbd_work_sz = MIPS3_PG_SIZE_MASK_TO_SIZE(MIPS3_PG_SIZE_1M);
		apbd_work_pa = ctob(physmem);

		mapped = mips3_wired_enter_page(apbd_work_va, apbd_work_pa,
		    apbd_work_sz);
		if (!mapped) {
			printf("%s: cannot allocate APbus PROM work\n",
			    __func__);
		}
	}
}

void *
apbus_device_to_hwaddr(struct apbus_dev *apbus_dev)
{
	struct apbus_ctl *ctl;

	if (apbus_dev == NULL)
		return NULL;

	ctl = apbus_dev->apbd_ctl;
	if (ctl == NULL)
		return NULL;

	return (void *)ctl->apbc_hwbase;
}

struct apbus_dev *
apbus_lookupdev(char *devname)
{
	struct apbus_dev *dp;

	dp = _sip->apbsi_dev;
	if (devname == NULL || *devname == '\0')
		return dp;

	/* search apbus_dev named 'devname' */
	while (dp) {
		if (strcmp(devname, dp->apbd_name) == 0)
			return dp;

		dp = dp->apbd_link;
	}

	return NULL;
}

static void
apctl_dump(struct apbus_ctl *apctl)
{
	if (!apctl)
		return;

	printf("	apbus_ctl dump (%p)\n", apctl);

	printf("	Num:		%d\n", apctl->apbc_ctlno);
	printf("	HWaddr:		0x%08x\n", apctl->apbc_hwbase);
	printf("	softc:		%p\n", apctl->apbc_softc);
	printf("	Slot:		%d\n", apctl->apbc_sl);
	printf("\n");

	if (apctl->apbc_link)
		apctl_dump(apctl->apbc_link);
}

void
apdevice_dump(struct apbus_dev *apdev)
{
	struct apbus_ctl *apctl;

	if (apdev == NULL)
		return;

	/* only no pseudo device */
	apctl = apdev->apbd_ctl;
	if (apctl == NULL || apctl->apbc_hwbase == 0)
		return;

	printf("apbus_dev dump (%p)\n", apdev);
	printf("name:		%s\n", apdev->apbd_name);
	printf("vendor:		%s\n", apdev->apbd_vendor);
	printf("atr:		%08x\n", apdev->apbd_atr);
	printf("rev:		%d\n", apdev->apbd_rev);
	printf("driver:		0x%08x\n", (unsigned int)apdev->apbd_driver);
	printf("ctl:		0x%08x\n", (unsigned int)apdev->apbd_ctl);
	printf("link:		0x%08x\n", (unsigned int)apdev->apbd_link);
	printf("\n");

	apctl_dump(apctl);
}
