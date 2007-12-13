/*-
 * Copyright (c) 2003
 *	Bill Paul <wpaul@windriver.com>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ndis_pci.c,v 1.8.8.1 2007/12/13 21:55:39 bouyer Exp $");
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/sys/dev/if_ndis/if_ndis_pci.c,v 1.8.2.3 2005/03/31 04:24:36 wpaul Exp $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#ifdef __FreeBSD__
#include <sys/module.h>
#else
#include <sys/lkm.h>
#endif
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_media.h>

#include <sys/bus.h>
#ifdef __FreeBSD__
#include <machine/resource.h>
#include <sys/bus.h>
#include <sys/rman.h>
#endif

#ifdef __NetBSD__
#include <sys/kthread.h>
#include <net/if_ether.h>
#endif

#include <net80211/ieee80211_var.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <compat/ndis/pe_var.h>
#include <compat/ndis/resource_var.h>
#include <compat/ndis/ntoskrnl_var.h>
#include <compat/ndis/ndis_var.h>
#include <compat/ndis/cfg_var.h>
#include <dev/if_ndis/if_ndisvar.h>

#include "ndis_driver_data.h"

#ifdef __NetBSD__
#ifndef NDIS_LKM
#include <compat/ndis/hal_var.h>
#endif
#endif /* __NetBSD__ */

#ifdef NDIS_PCI_DEV_TABLE 

#ifdef __FreeBSD
MODULE_DEPEND(ndis, pci, 1, 1, 1);
MODULE_DEPEND(ndis, ether, 1, 1, 1);
MODULE_DEPEND(ndis, wlan, 1, 1, 1);
MODULE_DEPEND(ndis, ndisapi, 1, 1, 1);
#endif

/*
 * Various supported device vendors/types and their names.
 * These are defined in the ndis_driver_data.h file.
 */
static struct ndis_pci_type ndis_devs[] = {
#ifdef NDIS_PCI_DEV_TABLE
	NDIS_PCI_DEV_TABLE
#endif
	{ 0, 0, 0, NULL }
};

#ifdef __FreeBSD__
static int ndis_probe_pci	(device_t);
static int ndis_attach_pci	(device_t);
#else /* __NetBSD__ */
/*static*/ int  ndis_probe_pci(struct device *parent, 
				struct cfdata *match,
				void *aux);
/*static*/ void ndis_attach_pci(struct device *parent,
				struct device *self,
				void *aux);
#endif
#ifdef __FreeBSD__
static struct resource_list *ndis_get_resource_list
				(device_t, device_t);
extern int ndisdrv_modevent	(module_t, int, void *);
#endif
#ifdef __FreeBSD__
extern int ndis_attach		(device_t);
#else /* __NetBSD__ */
extern void ndis_attach		(void *);
#endif
extern int ndis_shutdown	(device_t);
#ifdef __FreeBSD__
extern int ndis_detach		(device_t);
#else /* __NetBSD__ */
extern int ndis_detach		(device_t, int);
#endif
extern int ndis_suspend		(device_t);
extern int ndis_resume		(device_t);

#ifdef __NetBSD__
extern int ndis_intr(void *);
#endif

extern unsigned char drv_data[];

#ifndef NDIS_LKM
//static funcptr ndis_txeof_wrap;
//static funcptr ndis_rxeof_wrap;
//static funcptr ndis_linksts_wrap;
//static funcptr ndis_linksts_done_wrap;
#endif

#ifdef __FreeBSD__
static device_method_t ndis_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		ndis_probe_pci),
	DEVMETHOD(device_attach,	ndis_attach_pci),
	DEVMETHOD(device_detach,	ndis_detach),
	DEVMETHOD(device_shutdown,	ndis_shutdown),
	DEVMETHOD(device_suspend,	ndis_suspend),
	DEVMETHOD(device_resume,	ndis_resume),

	/* Bus interface */
	DEVMETHOD(bus_get_resource_list, ndis_get_resource_list),

	{ 0, 0 }
};

static driver_t ndis_driver = {
#ifdef NDIS_DEVNAME
	NDIS_DEVNAME,
#else
	"ndis",
#endif
	ndis_methods,
	sizeof(struct ndis_softc)
};

static devclass_t ndis_devclass;

#ifdef NDIS_MODNAME
#define NDIS_MODNAME_OVERRIDE_PCI(x)					\
	DRIVER_MODULE(x, pci, ndis_driver, ndis_devclass, ndisdrv_modevent, 0)
#define NDIS_MODNAME_OVERRIDE_CARDBUS(x)				\
	DRIVER_MODULE(x, cardbus, ndis_driver, ndis_devclass,		\
		ndisdrv_modevent, 0)
NDIS_MODNAME_OVERRIDE_PCI(NDIS_MODNAME);
NDIS_MODNAME_OVERRIDE_CARDBUS(NDIS_MODNAME);
#else
DRIVER_MODULE(ndis, pci, ndis_driver, ndis_devclass, ndisdrv_modevent, 0);
DRIVER_MODULE(ndis, cardbus, ndis_driver, ndis_devclass, ndisdrv_modevent, 0);
#endif

#endif  /* __FreeBSD__ */
#ifdef __NetBSD__

CFATTACH_DECL(
#ifdef NDIS_DEVNAME
	NDIS_DEVNAME,
#else
	ndis,
#endif
	sizeof(struct ndis_softc),
	ndis_probe_pci,
	ndis_attach_pci,
	ndis_detach,
	NULL);

#endif /* __NetBSD__ */

#ifdef __FreeBSD__
/*
 * Probe for an NDIS device. Check the PCI vendor and device
 * IDs against our list and return a device name if we find a match.
 */
static int
ndis_probe_pci(dev)
	device_t		dev;
{
	struct ndis_pci_type	*t;
	driver_object		*drv;

	t = ndis_devs;
	drv = windrv_lookup(0, "PCI Bus");

	if (drv == NULL)
		return(ENXIO);

	while(t->ndis_name != NULL) {
		if ((pci_get_vendor(dev) == t->ndis_vid) &&
		    (pci_get_device(dev) == t->ndis_did) &&
		    ((pci_read_config(dev, PCIR_SUBVEND_0, 4) ==
		    t->ndis_subsys) || t->ndis_subsys == 0)) {
			device_set_desc(dev, t->ndis_name);

			/* Create PDO for this device instance */
			windrv_create_pdo(drv, dev);
			return(0);
		}
		t++;
	}

	return(ENXIO);
}
#endif /* __FreeBSD__ */
#ifdef __NetBSD__

extern int
ndis_lkm_handle(struct lkm_table *lkmtp, int cmd);
extern int 
ndisdrv_modevent(module_t mod, int cmd);

/* 
 * These are just for the in-kernel version, to delay calling
 * these functions untill enough context is built up.
 */
void load_ndisapi(void *);
void load_ndisdrv(void *);

void load_ndisapi(void *arg)
{
	ndis_lkm_handle(NULL, MOD_LOAD);
}
void load_ndisdrv(void *arg)
{
	ndisdrv_modevent(NULL, MOD_LOAD);
}

/*static*/ int
ndis_probe_pci(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;
	int vendor  = PCI_VENDOR(pa->pa_id);
	int product = PCI_PRODUCT(pa->pa_id);
	
	struct ndis_pci_type *t = ndis_devs;
	driver_object        *drv = NULL;	/* = windrv_lookup(0, "PCI Bus");**/
		
#ifdef NDIS_DBG
	printf("in ndis_probe_pci\n");
	printf("vendor = %x, product = %x\n", vendor, product);
#endif
	
	while(t->ndis_name != NULL) {
#ifdef NDIS_DBG
			printf("t->ndis_vid = %x, t->ndis_did = %x\n",
			       t->ndis_vid, t->ndis_did);
#endif
		if((vendor  == t->ndis_vid) && (product == t->ndis_did)) {
#ifndef NDIS_LKM	
			ndis_lkm_handle(NULL, LKM_E_LOAD);
			//kthread_create(load_ndisapi, NULL);
#endif /* NDIS_LKM */
			ndisdrv_modevent(NULL, MOD_LOAD);
			//kthread_create(load_ndisdrv, NULL);
				
			drv = windrv_lookup(0, "PCI Bus");
			printf("Matching vendor: %x, product: %x, name: %s\n", vendor, product, t->ndis_name);
			windrv_create_pdo(drv, parent);
			return 1;
		}
		t++;
	}
	
	return 0;  /* dosen't match */
}
#endif /* __NetBSD__ */

#ifdef __FreeBSD__
/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static int
ndis_attach_pci(dev)
	device_t		dev;
{
	struct ndis_softc	*sc;
	int			unit, error = 0, rid;
	struct ndis_pci_type	*t;
	int			devidx = 0, defidx = 0;
	struct resource_list	*rl;
	struct resource_list_entry	*rle;

	sc = device_get_softc(dev);
	unit = device_get_unit(dev);
	sc->ndis_dev = dev;

	/*
	 * Map control/status registers.
	 */

	pci_enable_busmaster(dev);

	rl = BUS_GET_RESOURCE_LIST(device_get_parent(dev), dev);
	if (rl != NULL) {
		SLIST_FOREACH(rle, rl, link) {
			switch (rle->type) {
			case SYS_RES_IOPORT:
				sc->ndis_io_rid = rle->rid;
				sc->ndis_res_io = bus_alloc_resource(dev,
				    SYS_RES_IOPORT, &sc->ndis_io_rid,
				    0, ~0, 1, RF_ACTIVE);
				if (sc->ndis_res_io == NULL) {
					aprint_error("%s: couldn't map iospace\n",
						     dev->dv_xname);
					error = ENXIO;
					goto fail;
				}
				break;
			case SYS_RES_MEMORY:
				if (sc->ndis_res_altmem != NULL &&
				    sc->ndis_res_mem != NULL) {
					aprint_error("%s: too many memory resources\n"
						     dev->dv_xname);
					error = ENXIO;
					goto fail;
				}
				if (sc->ndis_res_mem) {
					sc->ndis_altmem_rid = rle->rid;
					sc->ndis_res_altmem =
					    bus_alloc_resource(dev,
					        SYS_RES_MEMORY,
						&sc->ndis_altmem_rid,
						0, ~0, 1, RF_ACTIVE);
					if (sc->ndis_res_altmem == NULL) {
						aprint_error("%s: couldn't map alt "
							     "memory\n",
							     dev->dv_xname);
						error = ENXIO;
						goto fail;
					}
				} else {
					sc->ndis_mem_rid = rle->rid;
					sc->ndis_res_mem =
					    bus_alloc_resource(dev,
					        SYS_RES_MEMORY,
						&sc->ndis_mem_rid,
						0, ~0, 1, RF_ACTIVE);
					if (sc->ndis_res_mem == NULL) {
						aprint_error("%s: couldn't map memory\n",
							     dev->dv_xname);
						error = ENXIO;
						goto fail;
					}
				}
				break;
			case SYS_RES_IRQ:
				rid = rle->rid;
				sc->ndis_irq = bus_alloc_resource(dev,
				    SYS_RES_IRQ, &rid, 0, ~0, 1,
	    			    RF_SHAREABLE | RF_ACTIVE);
				if (sc->ndis_irq == NULL) {
					aprint_error("%s: couldn't map interrupt\n"
						     dev->dv_xname);
					error = ENXIO;
					goto fail;
				}
				break;
			default:
				break;
			}
			sc->ndis_rescnt++;
		}
	}

	/*
	 * If the BIOS did not set up an interrupt for this device,
	 * the resource traversal code above will fail to set up
	 * an IRQ resource. This is usually a bad thing, so try to
	 * force the allocation of an interrupt here. If one was
	 * not assigned to us by the BIOS, bus_alloc_resource()
	 * should route one for us.
	 */
	if (sc->ndis_irq == NULL) {
		rid = 0;
		sc->ndis_irq = bus_alloc_resource(dev, SYS_RES_IRQ,
		    &rid, 0, ~0, 1, RF_SHAREABLE | RF_ACTIVE);
		if (sc->ndis_irq == NULL) {
			aprint_error("%s: couldn't route interrupt\n",
				     dev->dv_xname);
			error = ENXIO;
			goto fail;
		}
		sc->ndis_rescnt++;
	}

	/*
	 * Allocate the parent bus DMA tag appropriate for PCI.
	 */
#define NDIS_NSEG_NEW 32
	error = bus_dma_tag_create(NULL,	/* parent */
			1, 0,			/* alignment, boundary */
			BUS_SPACE_MAXADDR_32BIT,/* lowaddr */
                        BUS_SPACE_MAXADDR,	/* highaddr */
			NULL, NULL,		/* filter, filterarg */
			MAXBSIZE, NDIS_NSEG_NEW,/* maxsize, nsegments */
			BUS_SPACE_MAXSIZE_32BIT,/* maxsegsize */
			BUS_DMA_ALLOCNOW,       /* flags */
			NULL, NULL,		/* lockfunc, lockarg */
			&sc->ndis_parent_tag);

        if (error)
                goto fail;

	sc->ndis_iftype = PCIBus;

	/* Figure out exactly which device we matched. */

	t = ndis_devs;

	while(t->ndis_name != NULL) {
		if ((pci_get_vendor(dev) == t->ndis_vid) &&
		    (pci_get_device(dev) == t->ndis_did)) {
			if (t->ndis_subsys == 0)
				defidx = devidx;
			else {
				if (t->ndis_subsys ==
				    pci_read_config(dev, PCIR_SUBVEND_0, 4))
					break;
			}
		}
		t++;
		devidx++;
	}

	if (ndis_devs[devidx].ndis_name == NULL)
		sc->ndis_devidx = defidx;
	else
		sc->ndis_devidx = devidx;

	error = ndis_attach(dev);

fail:
	return(error);
}
#endif /* __FreeBSD__ */
#ifdef __NetBSD__
/* 6 BADR's + 1 IRQ  (so far) */
#define MAX_RESOURCES 7

/*static*/ 
void ndis_attach_pci(struct device *parent, struct device *self, void *aux)
{
	struct ndis_softc *sc = (struct ndis_softc*)self;
	struct pci_attach_args *pa = aux;
#ifdef NDIS_DBG       
	char devinfo[256];
#endif
	pci_intr_handle_t ih;
	pcireg_t type;
	bus_addr_t	base;
	bus_size_t	size;
	int		flags;
	ndis_resource_list 		*rl  = NULL;
	struct cm_partial_resource_desc	*prd = NULL;
#ifdef NDIS_DBG
	struct pci_conf_state conf_state;
	int revision, i;
#endif
	int bar;
	
	printf("in ndis_attach_pci()\n");

	/* initalize the softc */
	//sc->ndis_hardware_type  = NDIS_PCI;
	sc->ndis_dev		= self;
	sc->ndis_iftype 	= PCIBus;
	sc->ndis_res_pc		= pa->pa_pc;
	sc->ndis_res_pctag	= pa->pa_tag;
	/* TODO: is this correct? All are just pa->pa_dmat? */	
	sc->ndis_mtag		= pa->pa_dmat;
	sc->ndis_ttag		= pa->pa_dmat;
	sc->ndis_parent_tag 	= pa->pa_dmat;
	sc->ndis_res_io		= NULL;
	sc->ndis_res_mem	= NULL;
	sc->ndis_res_altmem	= NULL;
	sc->ndis_block 		= NULL;
	sc->ndis_shlist		= NULL;
	
	ndis_in_isr		= FALSE;
	
	printf("sc->ndis_mtag = %x\n", (unsigned int)sc->ndis_mtag);

	rl = malloc(sizeof(ndis_resource_list) +
	    (sizeof(cm_partial_resource_desc) * (MAX_RESOURCES-1)),
	    M_DEVBUF, M_NOWAIT|M_ZERO);

	if(rl == NULL) {
		sc->error = ENOMEM;
		//printf("error: out of memory\n");
		return;
	}
	
	rl->cprl_version = 5;
	rl->cprl_version = 1;    
	rl->cprl_count = 0;
	prd = rl->cprl_partial_descs;
	
#ifdef NDIS_DBG
        pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof devinfo);
        revision = PCI_REVISION(pa->pa_class);
        printf(": %s (rev. 0x%02x)\n", devinfo, revision);
	
	pci_conf_print(sc->ndis_res_pc, sc->ndis_res_pctag, NULL);

	pci_conf_capture(sc->ndis_res_pc, sc->ndis_res_pctag, &conf_state);
	for(i=0; i<16; i++) {
		printf("conf_state.reg[%d] = %x\n", i, conf_state.reg[i]);
	}
#endif
	
	/* just do the conversion work in attach instead of calling ndis_convert_res() */
	for(bar = 0x10; bar <= 0x24; bar += 0x04) {
		type = pci_mapreg_type(sc->ndis_res_pc, sc->ndis_res_pctag, bar);
		if(pci_mapreg_info(sc->ndis_res_pc, sc->ndis_res_pctag, bar, type, &base,
			&size, &flags)) {
			printf("pci_mapreg_info() failed on BAR 0x%x!\n", bar);
		} else {			
			switch(type) {
			case PCI_MAPREG_TYPE_IO:
				prd->cprd_type 				= CmResourceTypePort;
				prd->cprd_flags 			= CM_RESOURCE_PORT_IO;
				prd->u.cprd_port.cprd_start.np_quad 	= (uint64_t)base;
				prd->u.cprd_port.cprd_len  	  	= (uint32_t)size;
				if((sc->ndis_res_io = 
					malloc(sizeof(struct ndis_resource), M_DEVBUF, M_NOWAIT | M_ZERO)) == NULL) {
					//printf("error: out of memory\n");
					sc->error = ENOMEM;
					return;
				}
				sc->ndis_res_io->res_base = base;
				sc->ndis_res_io->res_size = size;
				sc->ndis_res_io->res_tag  = X86_BUS_SPACE_IO;
				bus_space_map(sc->ndis_res_io->res_tag,
					 sc->ndis_res_io->res_base,
					 sc->ndis_res_io->res_size,
					 flags,
					&sc->ndis_res_io->res_handle);
				break;
			case PCI_MAPREG_TYPE_MEM:
				prd->cprd_type 				= CmResourceTypeMemory;
				prd->cprd_flags 			= CM_RESOURCE_MEMORY_READ_WRITE;
				prd->u.cprd_mem.cprd_start.np_quad 	= (uint64_t)base;
				prd->u.cprd_mem.cprd_len		= (uint32_t)size;
				
				if(sc->ndis_res_mem != NULL && 
					sc->ndis_res_altmem != NULL) {
					printf("too many resources\n");
					sc->error = ENXIO;
					return;
				}
				if(sc->ndis_res_mem) {
					if((sc->ndis_res_altmem = 
						malloc(sizeof(struct ndis_resource), M_DEVBUF, M_NOWAIT | M_ZERO)) == NULL) {
						sc->error = ENOMEM;
						return;
					}
					sc->ndis_res_altmem->res_base = base;
					sc->ndis_res_altmem->res_size = size;
					sc->ndis_res_altmem->res_tag  = X86_BUS_SPACE_MEM;
					
					
					if(bus_space_map(sc->ndis_res_altmem->res_tag,
						sc->ndis_res_altmem->res_base,
						sc->ndis_res_altmem->res_size,
						flags|BUS_SPACE_MAP_LINEAR,
						&sc->ndis_res_altmem->res_handle)) {
							printf("bus_space_map failed\n");
					}
				} else {
					if((sc->ndis_res_mem = 
						malloc(sizeof(struct ndis_resource), M_DEVBUF, M_NOWAIT | M_ZERO)) == NULL) {
						sc->error = ENOMEM;
						return;
					}
					sc->ndis_res_mem->res_base = base;
					sc->ndis_res_mem->res_size = size;
					sc->ndis_res_mem->res_tag  = X86_BUS_SPACE_MEM;
					
					if(bus_space_map(sc->ndis_res_mem->res_tag,
						sc->ndis_res_mem->res_base,
						sc->ndis_res_mem->res_size,
						flags|BUS_SPACE_MAP_LINEAR,
						&sc->ndis_res_mem->res_handle)) {
							printf("bus_space_map failed\n");
					}
				}
				break;
											   
			default:
				printf("unknown type\n");
			}
			prd->cprd_sharedisp = CmResourceShareDeviceExclusive;

			rl->cprl_count++;								
			prd++;
		}
	}
	
	/* add the interrupt to the list */
	prd->cprd_type 	= CmResourceTypeInterrupt;
	prd->cprd_flags = 0;
	/* TODO: is this all we need to save for the interrupt? */
	prd->u.cprd_intr.cprd_level = pa->pa_intrline;
	prd->u.cprd_intr.cprd_vector = pa->pa_intrline;
	prd->u.cprd_intr.cprd_affinity = 0;
	rl->cprl_count++;
	
	pci_intr_map(pa, &ih);
	sc->ndis_intrhand = pci_intr_establish(pa->pa_pc, ih, IPL_NET /*| PCATCH*/, ndis_intr, sc);
	sc->ndis_irq = (void *)sc->ndis_intrhand;
	
	printf("pci interrupt: %s\n", pci_intr_string(pa->pa_pc, ih));
	
	if(rl == NULL) {
		sc->error = ENOMEM;
		return;
	}
	
	/* save resource list in the softc */
	sc->ndis_rl = rl;
	sc->ndis_rescnt = rl->cprl_count;
	
	kthread_create(ndis_attach, (void *)sc);
}
#endif /* __NetBSD__ */

#ifdef __FreeBSD__
static struct resource_list *
ndis_get_resource_list(dev, child)
	device_t		dev;
	device_t		child;
{
	struct ndis_softc	*sc;

	sc = device_get_softc(dev);
	return (BUS_GET_RESOURCE_LIST(device_get_parent(sc->ndis_dev), dev));
}
#endif /* __FreeBSD__ */

#endif /* NDIS_PCI_DEV_TABLE */
