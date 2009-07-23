/*	$NetBSD: mp.c,v 1.1.6.3 2009/07/23 23:31:37 jym Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mp.c,v 1.1.6.3 2009/07/23 23:31:37 jym Exp $");

#include "opt_multiprocessor.h"
#include "pchb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <machine/specialreg.h>
#include <machine/cpuvar.h>
#include <machine/bus.h>
#include <machine/mpconfig.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "pci.h"
#include "locators.h"

#if NPCI > 0
int
mp_pci_scan(device_t self, struct pcibus_attach_args *pba,
	        cfprint_t print)
{
	int i, cnt;
	struct mp_bus *mpb;

	cnt = 0;
	for (i = 0; i < mp_nbus; i++) {
		mpb = &mp_busses[i];
		if (mpb->mb_name == NULL)
			continue;
		if (strcmp(mpb->mb_name, "pci") == 0 && mpb->mb_dev == NULL) {
			pba->pba_bus = i;
			mpb->mb_dev =
			    config_found_ia(self, "pcibus", pba, print);
			cnt++;
		}
	}
	return cnt;
}

void
mp_pci_childdetached(device_t self, device_t child)
{
	int i;
	struct mp_bus *mpb;

	for (i = 0; i < mp_nbus; i++) {
		mpb = &mp_busses[i];
		if (mpb->mb_name == NULL)
			continue;
		if (strcmp(mpb->mb_name, "pci") == 0 && mpb->mb_dev == child)
			mpb->mb_dev = NULL;
	}
}
#endif
