/* $NetBSD: pvh_consinit.c,v 1.4 2023/07/22 19:13:17 mrg Exp $ */

/*
 * Copyright (c) 2020 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pvh_consinit.c,v 1.4 2023/07/22 19:13:17 mrg Exp $");

#include "xencons.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_prot.h>

#include <dev/cons.h>
#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/include/public/hvm/hvm_op.h>
#include <xen/include/public/hvm/params.h>

#include "xen_def_cons.h"

static int pvh_xenconscn_getc(dev_t);
static void pvh_xenconscn_putc(dev_t, int);
static void pvh_xenconscn_pollc(dev_t, int);

static struct consdev pvh_xencons = {
        NULL, NULL, pvh_xenconscn_getc, pvh_xenconscn_putc, pvh_xenconscn_pollc,
	NULL, NULL, NULL, NODEV, CN_NORMAL
};


int
xen_pvh_consinit(void)
{
	/*
	 * hugly hack because we're called multiple times at different
	 * boot stage.
	 */
	static int initted = 0;
	if (xendomain_is_dom0()) {
		union xen_cmdline_parseinfo xcp;
		xen_parse_cmdline(XEN_PARSE_CONSOLE, &xcp);
#ifdef CONS_OVERRIDE
                if (strcmp(default_consinfo.devname, "tty0") == 0 ||
		    strcmp(default_consinfo.devname, "pc") == 0) {
#else
		if (strcmp(xcp.xcp_console, "tty0") == 0 || /* linux name */
		    strcmp(xcp.xcp_console, "pc") == 0) { /* NetBSD name */
#endif /* CONS_OVERRIDE */
			return 0; /* native console code will do it */
		}
	}
	if (initted == 0 && !xendomain_is_dom0()) {
		/* pmap not up yet, fall back to printk() */
		cn_tab = &pvh_xencons;
		initted++;
		return 1;
	} else if (initted > 1) {
		return 1;
	}
	initted++;
	if (xendomain_is_dom0()) {
		/* we know we're using Xen's console at this point */
		xenconscn_attach(); /* no ring in this case */
		initted++; /* don't init console twice */
		return 1;
	}

#if NXENCONS > 0
	/* we can now map the xencons rings. */
	struct xen_hvm_param xen_hvm_param;


	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_CONSOLE_PFN;

	if ( HYPERVISOR_hvm_op(HVMOP_get_param, &xen_hvm_param) < 0) 
		panic("xen_pvh_consinit: can't get console PFN");

	xen_start_info.console.domU.mfn = xen_hvm_param.value;
	pmap_kenter_pa((vaddr_t) xencons_interface, ptoa(xen_hvm_param.value),
	    VM_PROT_READ|VM_PROT_WRITE, 0);

	xen_hvm_param.domid = DOMID_SELF;
	xen_hvm_param.index = HVM_PARAM_CONSOLE_EVTCHN;

	if ( HYPERVISOR_hvm_op(HVMOP_get_param, &xen_hvm_param) < 0)
		panic("xen_pvh_consinit: can't get console event");

	xen_start_info.console.domU.evtchn = xen_hvm_param.value;
	xenconscn_attach();
#endif
	return 1;
}

static int
pvh_xenconscn_getc(dev_t dev)
{
	while(1)
		;
	return -1;
}

static void
pvh_xenconscn_putc(dev_t dev, int c)
{
	printk("%c", c);
}

static void
pvh_xenconscn_pollc(dev_t dev, int on)
{
	return;
}
