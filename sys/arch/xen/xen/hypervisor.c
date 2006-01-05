/* $NetBSD: hypervisor.c,v 1.12.2.4 2006/01/05 05:28:11 riz Exp $ */

/*
 * Copyright (c) 2005 Manuel Bouyer.
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
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hypervisor.c,v 1.12.2.4 2006/01/05 05:28:11 riz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <dev/sysmon/sysmonvar.h>

#include "xencons.h"
#include "xennet.h"
#include "xbd.h"
#include "npx.h"
#include "isa.h"
#include "pci.h"

#include "opt_xen.h"

#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/evtchn.h>
#include <machine/ctrl_if.h>

#ifdef DOM0OPS
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/tree.h>
#include <sys/vnode.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>
#include <machine/kernfs_machdep.h>
#include <dev/isa/isavar.h>
#endif
#if NPCI > 0
#include <dev/pci/pcivar.h>
#endif

#if NXENNET > 0
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <machine/if_xennetvar.h>
#endif

#if NXBD > 0
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/bufq.h>
#include <dev/dkvar.h>
#include <machine/xbdvar.h>
#endif

int	hypervisor_match(struct device *, struct cfdata *, void *);
void	hypervisor_attach(struct device *, struct device *, void *);

CFATTACH_DECL(hypervisor, sizeof(struct device),
    hypervisor_match, hypervisor_attach, NULL, NULL);

static int hypervisor_print(void *, const char *);

union hypervisor_attach_cookie {
	const char *hac_device;		/* first elem of all */
#if NXENCONS > 0
	struct xencons_attach_args hac_xencons;
#endif
#if NXENNET > 0
	struct xennet_attach_args hac_xennet;
#endif
#if NXBD > 0
	struct xbd_attach_args hac_xbd;
#endif
#if NNPX > 0
	struct xen_npx_attach_args hac_xennpx;
#endif
};

/* 
 * This is set when the ISA bus is attached.  If it's not set by the
 * time it's checked below, then mainbus attempts to attach an ISA. 
 */   
#ifdef DOM0OPS
int     isa_has_been_seen;
#if NISA > 0
struct  x86_isa_chipset x86_isa_chipset;
#endif
#endif

/* shutdown/reboot message stuff */
static void hypervisor_shutdown_handler(ctrl_msg_t *, unsigned long);
static struct sysmon_pswitch hysw_shutdown = {
	.smpsw_type = PSWITCH_TYPE_POWER,
	.smpsw_name = "hypervisor",
};
static struct sysmon_pswitch hysw_reboot = {
	.smpsw_type = PSWITCH_TYPE_RESET,
	.smpsw_name = "hypervisor",
};

/*
 * Probe for the hypervisor; always succeeds.
 */
int
hypervisor_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct hypervisor_attach_args *haa = aux;

	if (strcmp(haa->haa_busname, "hypervisor") == 0)
		return 1;
	return 0;
}

/*
 * Attach the hypervisor.
 */
void
hypervisor_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if NPCI > 0
	struct pcibus_attach_args pba;
#if defined(DOM0OPS) && NISA > 0
	struct isabus_attach_args iba;
#endif
	physdev_op_t physdev_op;
	int i, j, busnum;
#endif
	union hypervisor_attach_cookie hac;

	printf("\n");

	init_events();

#if NXENCONS > 0
	hac.hac_xencons.xa_device = "xencons";
	config_found_ia(self, "xendevbus", &hac.hac_xencons, hypervisor_print);
#endif
#if NXENNET > 0
	hac.hac_xennet.xa_device = "xennet";
	xennet_scan(self, &hac.hac_xennet, hypervisor_print);
#endif
#if NXBD > 0
	hac.hac_xbd.xa_device = "xbd";
	xbd_scan(self, &hac.hac_xbd, hypervisor_print);
#endif
#if NNPX > 0
	hac.hac_xennpx.xa_device = "npx";
	config_found_ia(self, "xendevbus", &hac.hac_xennpx, hypervisor_print);
#endif
#if NPCI > 0

	physdev_op.cmd = PHYSDEVOP_PCI_PROBE_ROOT_BUSES;
	if ((i = HYPERVISOR_physdev_op(&physdev_op)) < 0) {
		printf("hypervisor: PHYSDEVOP_PCI_PROBE_ROOT_BUSES failed with status %d\n", i);
	} else {
#ifdef DEBUG
		printf("PCI_PROBE_ROOT_BUSES: ");
		for (i = 0; i < 256/32; i++)
			printf("0x%x ", physdev_op.u.pci_probe_root_buses.busmask[i]);
		printf("\n");
#endif
		memset(pci_bus_attached, 0, sizeof(u_int32_t) * 256 / 32);
		for (i = 0, busnum = 0; i < 256/32; i++) {
			u_int32_t mask = 
			    physdev_op.u.pci_probe_root_buses.busmask[i];
			for (j = 0; j < 32; j++, busnum++) {
				if ((mask & (1 << j)) == 0)
					continue;
				if (pci_bus_attached[i] & (1 << j)) {
					printf("bus %d already attached\n",
					    busnum);
					continue;
				}
				pba.pba_iot = X86_BUS_SPACE_IO;
				pba.pba_memt = X86_BUS_SPACE_MEM;
				pba.pba_dmat = &pci_bus_dma_tag;
				pba.pba_dmat64 = 0;
				pba.pba_flags = PCI_FLAGS_MEM_ENABLED |
						PCI_FLAGS_IO_ENABLED;
				pba.pba_bridgetag = NULL;
				pba.pba_bus = busnum;
				config_found_ia(self, "pcibus", &pba,
				    pcibusprint);
			}
		} 
	}
#if defined(DOM0OPS) && NISA > 0
	if (isa_has_been_seen == 0) {
		iba._iba_busname = "isa";
		iba.iba_iot = X86_BUS_SPACE_IO;
		iba.iba_memt = X86_BUS_SPACE_MEM;
		iba.iba_dmat = &isa_bus_dma_tag;
		iba.iba_ic = NULL; /* No isa DMA yet */
		config_found_ia(self, "isabus", &iba, isabusprint);
	}
#endif
#endif

#ifdef DOM0OPS
	if (xen_start_info.flags & SIF_PRIVILEGED) {
		xenkernfs_init();
		xenprivcmd_init();
		xen_shm_init();
		xbdback_init();
		xennetback_init();
	}
#endif
	if (sysmon_pswitch_register(&hysw_reboot) != 0 ||
	    sysmon_pswitch_register(&hysw_shutdown) != 0)
		printf("%s: unable to register with sysmon\n",
		    self->dv_xname);
	else 
		ctrl_if_register_receiver(CMSG_SHUTDOWN,
		    hypervisor_shutdown_handler, CALLBACK_IN_BLOCKING_CONTEXT);
}

static int
hypervisor_print(aux, parent)
	void *aux;
	const char *parent;
{
	union hypervisor_attach_cookie *hac = aux;

	if (parent)
		aprint_normal("%s at %s", hac->hac_device, parent);
	return (UNCONF);
}

void
hypervisor_notify_via_evtchn(unsigned int port)
{
	evtchn_op_t op;

	op.cmd = EVTCHNOP_send;
	op.u.send.local_port = port;
	(void)HYPERVISOR_event_channel_op(&op);
}

#ifdef DOM0OPS

#define DIR_MODE	(S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)

kernfs_parentdir_t *kernxen_pkt;

void
xenkernfs_init()
{
	kernfs_entry_t *dkt;

	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_DIR, "xen", NULL, KFSsubdir, VDIR, DIR_MODE);
	kernfs_addentry(NULL, dkt);
	kernxen_pkt = KERNFS_ENTOPARENTDIR(dkt);
}
#endif

/* handler for the shutdown messages */
static void
hypervisor_shutdown_handler(ctrl_msg_t *msg, unsigned long id)
{
	switch(msg->subtype) {
	case CMSG_SHUTDOWN_POWEROFF:	
		sysmon_pswitch_event(&hysw_shutdown, PSWITCH_EVENT_PRESSED);
		break;
	case CMSG_SHUTDOWN_REBOOT:	
		sysmon_pswitch_event(&hysw_reboot, PSWITCH_EVENT_PRESSED);
		break;
	default:
		printf("shutdown_handler: unknwon message %d\n",
		    msg->type);
	}
}
