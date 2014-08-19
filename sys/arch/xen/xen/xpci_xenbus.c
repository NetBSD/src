/*      $NetBSD: xpci_xenbus.c,v 1.11.2.2 2014/08/20 00:03:30 tls Exp $      */

/*
 * Copyright (c) 2009 Manuel Bouyer.
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
__KERNEL_RCSID(0, "$NetBSD: xpci_xenbus.c,v 1.11.2.2 2014/08/20 00:03:30 tls Exp $");

#include "opt_xen.h"


#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/bus_private.h>

#include <dev/isa/isareg.h>

#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/granttables.h>
#include <xen/xen-public/io/pciif.h>
#include <xen/xenbus.h>

#include "locators.h"

#include <dev/pci/pcivar.h>

#undef XPCI_DEBUG
#ifdef XPCI_DEBUG
#define DPRINTF(x) printf x;
#else
#define DPRINTF(x)
#endif

struct xpci_xenbus_softc {
	device_t sc_dev;
	struct xenbus_device *sc_xbusd;
	unsigned int sc_evtchn;
	int sc_backend_status; /* our status with backend */
#define XPCI_STATE_DISCONNECTED 0
#define XPCI_STATE_CONNECTED    1
#define XPCI_STATE_SUSPENDED    2
	int sc_shutdown;
	struct xen_pci_sharedinfo *sc_shared;
	grant_ref_t sc_shared_gntref;
};
#define GRANT_INVALID_REF -1

static int  xpci_xenbus_match(device_t, cfdata_t, void *);
static void xpci_xenbus_attach(device_t, device_t, void *);
static int  xpci_xenbus_detach(device_t, int);

static void xpci_backend_changed(void *, XenbusState);
static int  xpci_xenbus_resume(void *);
static void xpci_connect(struct xpci_xenbus_softc *);
static void xpci_attach_pcibus(int, int);

static struct xpci_xenbus_softc *xpci_sc = NULL;

CFATTACH_DECL_NEW(xpci_xenbus, sizeof(struct xpci_xenbus_softc),
   xpci_xenbus_match, xpci_xenbus_attach, xpci_xenbus_detach, NULL);

struct x86_bus_dma_tag pci_bus_dma_tag = {
	._tag_needs_free	= 0,
#if defined(_LP64) || defined(PAE)
	._bounce_thresh		= PCI32_DMA_BOUNCE_THRESHOLD,
	._bounce_alloc_lo	= 0,
	._bounce_alloc_hi	= PCI32_DMA_BOUNCE_THRESHOLD,
#else
	._bounce_thresh		= 0,
	._bounce_alloc_lo	= 0,
	._bounce_alloc_hi	= 0,
#endif
	._may_bounce		= NULL,
};

#ifdef _LP64
struct x86_bus_dma_tag pci_bus_dma64_tag = {
	._tag_needs_free	= 0,
	._bounce_thresh		= 0,
	._bounce_alloc_lo	= 0,
	._bounce_alloc_hi	= 0,
	._may_bounce		= NULL,
};
#endif

static int
xpci_xenbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct xenbusdev_attach_args *xa = aux;

	if (strcmp(xa->xa_type, "pci") != 0)
		return 0;

	if (match->cf_loc[XENBUSCF_ID] != XENBUSCF_ID_DEFAULT &&
	   match->cf_loc[XENBUSCF_ID] != xa->xa_id)
		return 0;

	return 1;
}

static void
xpci_xenbus_attach(device_t parent, device_t self, void *aux)
{
	struct xpci_xenbus_softc *sc = device_private(self);
	struct xenbusdev_attach_args *xa = aux;
#ifdef XBD_DEBUG
	char **dir, *val;
	int dir_n = 0;
	char id_str[20];
	int err;
#endif

	if (xpci_sc != NULL) {
		aprint_error("Xen PCI frontend already attached\n");
		return;
	}
	xpci_sc = sc;
	DPRINTF(("xpci_sc %p\n", xpci_sc));

	config_pending_incr(self);
	aprint_normal(": Xen PCI passthrough Interface\n");
	sc->sc_dev = self;

	sc->sc_xbusd = xa->xa_xbusd;
	sc->sc_xbusd->xbusd_otherend_changed = xpci_backend_changed;

	sc->sc_backend_status = XPCI_STATE_DISCONNECTED;
	sc->sc_shutdown = 1;
	/* initialise shared structures and tell backend that we are ready */
	xpci_xenbus_resume(sc);
}

static int
xpci_xenbus_detach(device_t dev, int flags)
{
	return EBUSY;
}

static int
xpci_xenbus_resume(void *p)
{
	struct xpci_xenbus_softc *sc = p;
	struct xenbus_transaction *xbt;
	int error;
	struct xen_pci_sharedinfo *shared;
	paddr_t ma;
	const char *errmsg;

	sc->sc_shared_gntref = GRANT_INVALID_REF;
	/* setup device: alloc event channel and shared info structure */
	sc->sc_shared = shared = (void *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
		 UVM_KMF_ZERO | UVM_KMF_WIRED);
	if (shared == NULL)
		 panic("xpci_xenbus_resume: can't alloc shared info");

	(void)pmap_extract_ma(pmap_kernel(), (vaddr_t)shared, &ma);
	error = xenbus_grant_ring(sc->sc_xbusd, ma, &sc->sc_shared_gntref);
	if (error)
		 return error;
	DPRINTF(("shared %p ma 0x%jx ref 0x%u\n", shared, (uintmax_t)ma,
	    sc->sc_shared_gntref));
	error = xenbus_alloc_evtchn(sc->sc_xbusd, &sc->sc_evtchn);
	if (error)
		 return error;
	aprint_verbose_dev(sc->sc_dev, "using event channel %d\n",
	    sc->sc_evtchn);
#if 0
	event_set_handler(sc->sc_evtchn, &xpci_handler, sc,
	    IPL_BIO, device_xname(sc->sc_dev));
#endif

again:
	xbt = xenbus_transaction_start();
	if (xbt == NULL)
		 return ENOMEM;
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "pci-op-ref","%u", sc->sc_shared_gntref);
	if (error) {
		 errmsg = "writing pci-op-ref";
		 goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "event-channel", "%u", sc->sc_evtchn);
	if (error) {
		 errmsg = "writing event channel";
		 goto abort_transaction;
	}
	error = xenbus_printf(xbt, sc->sc_xbusd->xbusd_path,
	    "magic", "%s", XEN_PCI_MAGIC);
	if (error) {
		 errmsg = "writing magic";
		 goto abort_transaction;
	}
	error = xenbus_switch_state(sc->sc_xbusd, xbt, XenbusStateInitialised);
	if (error) {
		 errmsg = "writing frontend XenbusStateInitialised";
		 goto abort_transaction;
	}
	error = xenbus_transaction_end(xbt, 0);
	if (error == EAGAIN)
		 goto again;
	if (error) {
		 xenbus_dev_fatal(sc->sc_xbusd, error, "completing transaction");
		 return -1;
	}
	return 0;

abort_transaction:
	xenbus_transaction_end(xbt, 1);
	xenbus_dev_fatal(sc->sc_xbusd, error, "%s", errmsg);
	return error;
}

static void
xpci_backend_changed(void *arg, XenbusState new_state)
{
	struct xpci_xenbus_softc *sc = device_private((device_t)arg);
	int s;
	DPRINTF(("%s: new backend state %d\n", device_xname(sc->sc_dev), new_state));

	switch (new_state) {
	case XenbusStateUnknown:
	case XenbusStateInitialising:
	case XenbusStateInitWait:
	case XenbusStateInitialised:
		break;
	case XenbusStateClosing:
		s = splbio();
		sc->sc_shutdown = 1;
		/* wait for requests to complete */
#if 0
		while (sc->sc_backend_status == XPCI_STATE_CONNECTED &&
		   sc->sc_dksc.sc_dkdev.dk_stats->io_busy > 0)
			tsleep(xpci_xenbus_detach, PRIBIO, "xpcidetach",
			   hz/2);
#endif
		splx(s);
		xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateClosed);
		break;
	case XenbusStateConnected:
		/*
		* note that xpci_backend_changed() can only be called by
		* the xenbus thread.
		*/

		if (sc->sc_backend_status == XPCI_STATE_CONNECTED)
			/* already connected */
			return;

		sc->sc_shutdown = 0;
		xpci_connect(sc);

		sc->sc_backend_status = XPCI_STATE_CONNECTED;

		/* the devices should be working now */
		config_pending_decr(sc->sc_dev);
		break;
	default:
		panic("bad backend state %d", new_state);
	}
}

static void
xpci_connect(struct xpci_xenbus_softc *sc)
{
	u_long num_roots;
	int err;
	char *string;
	char *domain, *bus, *ep;
	char node[10];
	u_long busn;
	int i;
	int s;

	err = xenbus_read_ul(NULL, sc->sc_xbusd->xbusd_otherend,
	   "root_num", &num_roots, 10);
	if (err == ENOENT) {
		aprint_error_dev(sc->sc_dev,
		   "No PCI Roots found, trying 0000:00\n");
		s = splhigh();
		xpci_attach_pcibus(0, 0);
		splx(s);
		xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateConnected);
		return;
	} else if (err) {
		aprint_error_dev(sc->sc_dev, "can't read root_num: %d\n", err);
		return;
	}
	aprint_verbose_dev(sc->sc_dev, "%lu bus%s\n", num_roots,
	    (num_roots > 1) ? "ses" : "");
	for (i = 0; i < num_roots; i++) {
		snprintf(node, sizeof(node), "root-%d", i);
		xenbus_read(NULL, sc->sc_xbusd->xbusd_otherend, node,
		    NULL, &string);
		/* split dddd:bb in 2 strings, a la strtok */
		domain = string;
		string[4] = '\0';
		bus = &string[5];
		if (strcmp(domain, "0000") != 0) {
			aprint_error_dev(sc->sc_dev,
			   "non-zero PCI domain %s not supported\n", domain);
		} else {
			busn = strtoul(bus, &ep, 16);
			if (*ep != '\0')
				aprint_error_dev(sc->sc_dev,
				   "%s is not a number\n", bus);
			else {
				s = splhigh();
				xpci_attach_pcibus(0, busn);
				splx(s);
			}
		}
		free(string, M_DEVBUF);
	}
	xenbus_switch_state(sc->sc_xbusd, NULL, XenbusStateConnected);
}

static void
xpci_attach_pcibus(int domain, int busn)
{
	struct pcibus_attach_args pba;

	memset(&pba, 0, sizeof(struct pcibus_attach_args));
	pba.pba_iot = x86_bus_space_io;
	pba.pba_memt = x86_bus_space_mem;
	pba.pba_dmat = &pci_bus_dma_tag;
#ifdef _LP64
	pba.pba_dmat64 = &pci_bus_dma64_tag;
#else
	pba.pba_dmat64 = NULL;
#endif /* _LP64 */
	pba.pba_flags = PCI_FLAGS_MEM_OKAY | PCI_FLAGS_IO_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	pba.pba_bridgetag = NULL;
	pba.pba_bus = busn;
	config_found_ia(xpci_sc->sc_dev, "pcibus", &pba, pcibusprint);
}

/* functions required by the MI PCI system */

void
pci_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{
	/* nothing */
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	return (32);
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	pcitag_t tag;
	KASSERT((function & ~0x7) == 0);
	KASSERT((device & ~0x1f) == 0);
	KASSERT((bus & ~0xff) == 0);
	tag.mode1 = (bus << 8) | (device << 3) | (function << 0);
	return tag;
}

void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag,
    int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag.mode1 >> 8) & 0xff;
	if (dp != NULL)
		*dp = (tag.mode1 >> 3) & 0x1f;
	if (fp != NULL)
		*fp = (tag.mode1 >> 0) & 0x7;
	return;
}

static void
xpci_do_op(struct xen_pci_op *op)
{

	struct xen_pci_op *active_op = &xpci_sc->sc_shared->op;
	static 	__cpu_simple_lock_t pci_conf_lock = __SIMPLELOCK_UNLOCKED;
	int s;

	s = splhigh();
	__cpu_simple_lock(&pci_conf_lock);
	memcpy(active_op, op, sizeof(struct xen_pci_op));
	x86_sfence();
	xen_atomic_set_bit(&xpci_sc->sc_shared->flags, _XEN_PCIF_active);
	hypervisor_notify_via_evtchn(xpci_sc->sc_evtchn);
	while (xen_atomic_test_bit(&xpci_sc->sc_shared->flags,
	   _XEN_PCIF_active)) {
		hypervisor_clear_event(xpci_sc->sc_evtchn);
		/* HYPERVISOR_yield(); */
	}
	memcpy(op, active_op, sizeof(struct xen_pci_op));
	__cpu_simple_unlock(&pci_conf_lock);
	splx(s);
}

static int
xpci_conf_read( pci_chipset_tag_t pc, pcitag_t tag, int reg, int size,
    pcireg_t *value)
{
	int bus, dev, func;
	struct xen_pci_op op = {
		.cmd    = XEN_PCI_OP_conf_read,
		.domain = 0, /* XXX */
	};
	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	DPRINTF(("pci_conf_read %d:%d:%d reg 0x%x", bus, dev, func, reg));
	op.bus = bus;
	op.devfn = (dev << 3) | func;
	op.offset = reg;
	op.size   = size;
	xpci_do_op(&op);
	*value = op.value;
	DPRINTF((" val 0x%x err %d\n", *value, op.err));
	return op.err;
}

pcireg_t
pci_conf_read( pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	static pcireg_t value, v;
	/*
	 * deal with linux stupidity: linux backend doesn't allow
	 * 4-byte read on some registers :(
	 */
	switch(reg) {
#if 0
	case 0x0004:
		xpci_conf_read(pc, tag, reg, 2, &v);
		value = v;
		xpci_conf_read(pc, tag, reg + 2, 2, &v);
		value |= v << 16;
		return value;
	case 0x003c:
	case 0x000c:
		value = xpci_conf_read(pc, tag, reg, 1, &v);
		value = v;
		xpci_conf_read(pc, tag, reg + 1, 1, &v);
		value |= v << 8;
		xpci_conf_read(pc, tag, reg + 2, 1, &v);
		value |= v << 16;
		xpci_conf_read(pc, tag, reg + 3, 1, &v);
		value |= v << 24;
		return value;
#endif
	default:
		xpci_conf_read(pc, tag, reg, 4, &v);
		value = v;
		return value;
	}
}

static int
xpci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, int size,
    pcireg_t data)
{
	int bus, dev, func;
	struct xen_pci_op op = {
		.cmd    = XEN_PCI_OP_conf_write,
		.domain = 0, /* XXX */
	};
	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	DPRINTF(("pci_conf_write %d:%d:%d reg 0x%x val 0x%x", bus, dev, func, reg, data));
	op.bus = bus;
	op.devfn = (dev << 3) | func;
	op.offset = reg;
	op.size   = size;
	op.value = data;
	xpci_do_op(&op);
	DPRINTF((" err %d\n", op.err));
	return op.err;
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	/* as for read we have to work around linux limitations */
	switch (reg) {
#if 0
	case 0x0004:
		xpci_conf_write(pc, tag, reg    , 2, data & 0xffff);
		xpci_conf_write(pc, tag, reg + 2, 2, (data >> 16) & 0xffff);
		return;
	case 0x003c:
	case 0x000c:
		xpci_conf_write(pc, tag, reg    , 1, data & 0xff);
		xpci_conf_write(pc, tag, reg + 1, 1, (data >> 8) & 0xff);
		xpci_conf_write(pc, tag, reg + 2, 1, (data >> 16) & 0xff);
		xpci_conf_write(pc, tag, reg + 3, 1, (data >> 24) & 0xff);
		return;
#endif
	default:
		xpci_conf_write(pc, tag, reg, 4, data);
	}
}

int
xpci_enumerate_bus(struct pci_softc *sc, const int *locators,
    int (*match)(const struct pci_attach_args *), struct pci_attach_args *pap)
{
#if 0
	char *string;
	char *domain, *bus, *dev, *func, *ep;
	u_long busn, devn, funcn;
	char node[10];
	u_long num_devs;
	int i;
	int err;
	pcitag_t tag;
	pci_chipset_tag_t pc = sc->sc_pc;

	err = xenbus_read_ul(NULL, xpci_sc->sc_xbusd->xbusd_otherend,
	   "num_devs", &num_devs, 10);
	if (err) {
		aprint_error_dev(xpci_sc->sc_dev,
		   "can't read num_devs: %d\n", err);
		return err;
	}
	for (i = 0; i < num_devs; i++) {
		snprintf(node, sizeof(node), "dev-%d", i);
		xenbus_read(NULL, xpci_sc->sc_xbusd->xbusd_otherend,
		   node, NULL, &string);
		/* split dddd:bb:dd:ff in 4 strings, a la strtok */
		domain = string;
		string[4] = '\0';
		bus = &string[5];
		string[7] = '\0';
		dev = &string[8];
		string[10] = '\0';
		func = &string[11];
		if (strcmp(domain, "0000") != 0) {
			aprint_error_dev(xpci_sc->sc_dev,
			   "non-zero PCI domain %s not supported\n", domain);
		} else {
			busn = strtoul(bus, &ep, 16);
			if (*ep != '\0') {
				aprint_error_dev(xpci_sc->sc_dev,
				   "%s is not a number\n", bus);
				goto endfor;
			}
			devn = strtoul(dev, &ep, 16);
			if (*ep != '\0') {
				aprint_error_dev(xpci_sc->sc_dev,
				   "%s is not a number\n", dev);
				goto endfor;
			}
			funcn = strtoul(func, &ep, 16);
			if (*ep != '\0') {
				aprint_error_dev(xpci_sc->sc_dev,
				   "%s is not a number\n", func);
				goto endfor;
			}
			if (busn != sc->sc_bus)
				goto endfor;
			tag = pci_make_tag(pc, busn, devn, funcn);
			err = pci_probe_device(sc, tag, match, pap);
			if (match != NULL && err != 0)
				return (err);
		}
endfor:
		free(string, M_DEVBUF);
	}
	return (0);
#else
	int devn, funcn;
	pcitag_t tag;
	pci_chipset_tag_t pc = sc->sc_pc;
	int err;
	/*
	 * Xen is lacking an important info: the domain:bus:dev:func
	 * present in dev-xx in the store are real PCI bus:dev:func, and
	 * xenback may present us fake ones, and unfortunably it's not
	 * the list of devices is not published in the store with this
	 * information. So we have to scan all dev/func combination :(
	 * the MI scan function isn't enough because it doesn't search
	 * for functions >= 1 if function 0 is not there.
	 */
	for (devn = 0; devn < 32; devn++) {
		for (funcn = 0; funcn < 8; funcn++) {
			pcireg_t csr, bar;
			int reg;
			tag = pci_make_tag(pc, sc->sc_bus, devn, funcn);
			/* try a READ on device ID. if it fails, no device */
			if (xpci_conf_read(pc, tag, PCI_ID_REG, 4, &csr) != 0)
				continue;
			/* check CSR. linux disable the device, sigh */
			if (xpci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG, 4,
			    &csr) != 0) {
				aprint_error(
				    "0x%2x:0x%2x:0x%2x failed to read CSR\n",
				    sc->sc_bus, devn, funcn);
				continue;
			}
			if ((csr &
			    (PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE))
			    == 0) {
				/* need to enable the device */
				for (reg = PCI_MAPREG_START;
				    reg < PCI_MAPREG_END;
				    reg += 4) {
					if (xpci_conf_read(pc, tag, reg, 4,
					    &bar) != 0) {
						aprint_error(
						    "0x%2x:0x%2x:0x%2x "
						    "failed to read 0x%x\n",
						    sc->sc_bus, devn, funcn,
						    reg);
						goto next;

					}
					if (bar & PCI_MAPREG_TYPE_IO)
						csr |= PCI_COMMAND_IO_ENABLE;
					else if (bar)
						csr |= PCI_COMMAND_MEM_ENABLE;
				}
				DPRINTF(("write CSR 0x%x\n", csr));
				if (xpci_conf_write(pc, tag,
				    PCI_COMMAND_STATUS_REG, 4, csr)) {
					aprint_error(
					    "0x%2x:0x%2x:0x%2x "
					    "failed to write CSR\n",
					     sc->sc_bus, devn, funcn);
					goto next;
				}
			}
			err = pci_probe_device(sc, tag, match, pap);
			if (match != NULL && err != 0)
				return (err);
next:
			continue;
		}
	}
	return 0;
#endif
}
