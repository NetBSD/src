/*      $NetBSD: pciback.c,v 1.4.6.2 2009/10/03 23:54:05 snj Exp $      */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pciback.c,v 1.4.6.2 2009/10/03 23:54:05 snj Exp $");

#include "opt_xen.h"
#include "rnd.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <machine/bus_private.h>

#include <dev/isa/isareg.h>

#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/granttables.h>
#include <xen/xen3-public/io/pciif.h>
#include <xen/xenbus.h>

#include <sys/stat.h>
#include <sys/dirent.h>
#include <miscfs/specfs/specdev.h>
#include <miscfs/kernfs/kernfs.h>
#include <xen/kernfs_machdep.h>

#include "locators.h"

#include <dev/pci/pcivar.h>
#include <machine/i82093var.h>

struct pciback_pci_softc;
struct pb_xenbus_instance;
/* list of devices we handle */
struct pciback_pci_dev {
	SLIST_ENTRY(pciback_pci_dev) pb_devlist_next; /* global list of pbd */
	SLIST_ENTRY(pciback_pci_dev) pb_guest_next; /* per-guest list of pbd */
	u_int pb_bus; /* our location */
	u_int pb_device;
	u_int pb_function;
	pci_chipset_tag_t pb_pc;
	pcitag_t pb_tag;
	struct pciback_pci_softc *pb_pci_softc;
	struct pb_xenbus_instance *pbx_instance;
};

/* list of devices we want to match */
SLIST_HEAD(pciback_pci_devlist, pciback_pci_dev) pciback_pci_devlist_head  =
    SLIST_HEAD_INITIALIZER(pciback_pci_devlist_head);
	
/* PCI-related functions and definitions  */

#define PCI_NBARS	((PCI_MAPREG_END - PCI_MAPREG_START) / 4)

struct pciback_pci_softc {
	device_t sc_dev;
	void *sc_ih; /* our interrupt; */
	struct pciback_pci_dev *sc_pb; /* our location */
	struct pci_bar {
		bus_space_tag_t b_t;
		bus_space_handle_t b_h;
		bus_addr_t b_addr;
		bus_size_t b_size;
		int b_type;
		int b_valid;
	} sc_bars[PCI_NBARS];
	pci_intr_handle_t sc_intrhandle;
	int  sc_irq;
	char sc_kernfsname[16];
};

int pciback_pci_match(device_t, cfdata_t, void *);
void pciback_pci_attach(device_t, device_t, void *);
static struct pciback_pci_dev* pciback_pci_lookup(u_int, u_int, u_int);
static void pciback_pci_init(void);

static int  pciback_parse_pci(const char *, u_int *, u_int *, u_int *);

/* kernfs-related functions and definitions */

static kernfs_parentdir_t *pciback_kern_pkt;
static int  pciback_kernfs_read(void *);

#define DIR_MODE	(S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#define FILE_MODE	(S_IRUSR)
static const struct kernfs_fileop pciback_dev_fileops[] = {
    { .kf_fileop = KERNFS_FILEOP_READ, .kf_vop = pciback_kernfs_read },
};

/* xenbus-related functions and definitions */

static int  pciback_xenbus_create(struct xenbus_device *);
static int  pciback_xenbus_destroy(void *);
static void pciback_xenbus_frontend_changed(void *, XenbusState);
static struct pb_xenbus_instance * pbxif_lookup(domid_t);
static void pciback_xenbus_export_device(struct pb_xenbus_instance *, char *);
static void pciback_xenbus_export_roots(struct pb_xenbus_instance *);

static int  pciback_xenbus_evthandler(void *);

/* emulate byte and word access to PCI config space */
static inline u_int8_t
pciback_read_byte(pci_chipset_tag_t pc, pcitag_t pa, int reg)
{

        return (pci_conf_read(pc, pa, (reg & ~0x03)) >>
            ((reg & 0x03) * 8) & 0xff);
}

static inline u_int16_t
pciback_read_word(pci_chipset_tag_t pc, pcitag_t pa, int reg)
{
        return (pci_conf_read(pc, pa, (reg & ~0x03)) >>
            ((reg & 0x03) * 8) & 0xffff);
}

static inline void
pciback_write_byte(pci_chipset_tag_t pc, pcitag_t pa, int reg, uint8_t val)
{
        pcireg_t pcival;

        pcival = pci_conf_read(pc, pa, (reg & ~0x03));
        pcival &= ~(0xff << ((reg & 0x03) * 8));
        pcival |= (val << ((reg & 0x03) * 8));
        pci_conf_write(pc, pa, (reg & ~0x03), pcival);
}

static inline void
pciback_write_word(pci_chipset_tag_t pc, pcitag_t pa, int reg, uint16_t val)
{
        pcireg_t pcival;

        pcival = pci_conf_read(pc, pa, (reg & ~0x03));
        pcival &= ~(0xffff << ((reg & 0x03) * 8));
        pcival |= (val << ((reg & 0x03) * 8));
        pci_conf_write(pc, pa, (reg & ~0x03), pcival);
}



CFATTACH_DECL_NEW(pciback, sizeof(struct pciback_pci_softc),
    pciback_pci_match, pciback_pci_attach, NULL, NULL);

static int pciback_pci_inited = 0;

/* a xenbus PCI backend instance */
struct pb_xenbus_instance {
	SLIST_ENTRY(pb_xenbus_instance) pbx_next; /* list of backend instances*/
	struct xenbus_device *pbx_xbusd;
	domid_t pbx_domid;
	struct pciback_pci_devlist pbx_pb_pci_dev; /* list of exported PCi devices */
	/* communication with the domU */
        unsigned int pbx_evtchn; /* our even channel */
        struct xen_pci_sharedinfo *pbx_sh_info;
        grant_handle_t pbx_shinfo_handle; /* to unmap shared page */
};

SLIST_HEAD(, pb_xenbus_instance) pb_xenbus_instances;

static struct xenbus_backend_driver pci_backend_driver = {
        .xbakd_create = pciback_xenbus_create,
        .xbakd_type = "pci"
};

int
pciback_pci_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	if (pciback_pci_inited == 0) {
		pciback_pci_init();
		pciback_pci_inited = 1;
	}
	if (pciback_pci_lookup(pa->pa_bus, pa->pa_device, pa->pa_function))
		return 500; /* we really want to take over anything else */
	return 0;
}

void
pciback_pci_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pciback_pci_softc *sc = device_private(self);
	char devinfo[256];
	int i;
	const char *intrstr;
	kernfs_entry_t *dkt;
	kfstype kfst;
	pcireg_t reg;

	sc->sc_dev = self;
	sc->sc_pb = pciback_pci_lookup(pa->pa_bus, pa->pa_device, pa->pa_function);
	if (sc->sc_pb == NULL)
		panic("pciback_pci_attach: pciback_lookup");
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo,
	    PCI_REVISION(pa->pa_class));
	sc->sc_pb->pb_pci_softc = sc;
	sc->sc_pb->pb_pc = pa->pa_pc;
	sc->sc_pb->pb_tag = pa->pa_tag;

	for (i = 0; i < PCI_NBARS;) {
		sc->sc_bars[i].b_type = pci_mapreg_type(pa->pa_pc, pa->pa_tag,
		    PCI_MAPREG_START + i * 4);
		if (pci_mapreg_map(pa, PCI_MAPREG_START + i * 4,
		    sc->sc_bars[i].b_type, 0,
		    &sc->sc_bars[i].b_t, &sc->sc_bars[i].b_h,
		    &sc->sc_bars[i].b_addr, &sc->sc_bars[i].b_size) == 0)
			sc->sc_bars[i].b_valid = 1;
		if (sc->sc_bars[i].b_valid) {
			aprint_verbose_dev(self, "%s: 0x%08jx - 0x%08jx\n",
			    (sc->sc_bars[i].b_type == PCI_MAPREG_TYPE_IO) ?
			    "I/O" : "mem",
			    (uintmax_t)sc->sc_bars[i].b_addr,
			    (uintmax_t)sc->sc_bars[i].b_size);
		}
			    
		if (sc->sc_bars[i].b_type ==
		    (PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT))
			i += 2;
		else
			i += 1;
	}
	/* map the irq so interrupt routing is done */
	if (pci_intr_map(pa, &sc->sc_intrhandle) != 0) {
		aprint_error_dev(self, "couldn't map interrupt\n");
	} else {
		intrstr = pci_intr_string(pa->pa_pc, sc->sc_intrhandle);
		aprint_normal_dev(self, "interrupting at %s\n",
		    intrstr ? intrstr : "unknown interrupt");
	}
	if (sc->sc_intrhandle.pirq & APIC_INT_VIA_APIC) {
		sc->sc_irq = APIC_IRQ_PIN(sc->sc_intrhandle.pirq);
	} else {
		sc->sc_irq = APIC_IRQ_LEGACY_IRQ(sc->sc_intrhandle.pirq);
	}
	/* XXX should be done elsewhere ? */
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_INTERRUPT_REG);
	reg &= ~ (PCI_INTERRUPT_LINE_MASK << PCI_INTERRUPT_LINE_SHIFT);
	reg |= (sc->sc_irq << PCI_INTERRUPT_LINE_SHIFT);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_INTERRUPT_REG, reg);
	printf("irq line %d pin %d sc %p\n",
	    PCI_INTERRUPT_LINE(pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_INTERRUPT_REG)),
	    PCI_INTERRUPT_PIN(pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_INTERRUPT_REG)), sc);
	/*
	 * don't establish the interrupt, we're not interested in
	 * getting it here. 
	 */
	/* publish the informations about this device to /kern/xen/pci */
	snprintf(sc->sc_kernfsname, sizeof(sc->sc_kernfsname),
	    "0000:%02x:%02x.%x", pa->pa_bus, pa->pa_device, pa->pa_function);
	kfst = KERNFS_ALLOCTYPE(pciback_dev_fileops);
	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_REG, sc->sc_kernfsname, sc,
	    kfst, VREG, FILE_MODE);
	kernfs_addentry(pciback_kern_pkt, dkt);
}

static int
pciback_kernfs_read(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct kernfs_node *kfs = VTOKERN(ap->a_vp);
	struct uio *uio = ap->a_uio;
	struct pciback_pci_softc *sc = kfs->kfs_kt->kt_data;
#define PCIBACK_KERNFS_SIZE 512
	static char buf[PCIBACK_KERNFS_SIZE];
	off_t off;
	off_t len;
	int error, i;

	off = uio->uio_offset;
	len = 0;
	for(i = 0; i < PCI_NBARS; i++) {
		if (sc->sc_bars[i].b_valid) {
			len += snprintf(&buf[len], PCIBACK_KERNFS_SIZE - len,
			    "%s: 0x%08jx - 0x%08jx\n",
			    (sc->sc_bars[i].b_type == PCI_MAPREG_TYPE_IO) ?
			    "I/O" : "mem",
			    (uintmax_t)sc->sc_bars[i].b_addr,
			    (uintmax_t)(sc->sc_bars[i].b_addr + sc->sc_bars[i].b_size));
		}
	}
	len += snprintf(&buf[len], PCIBACK_KERNFS_SIZE - len,
	    "irq: %d\n", sc->sc_irq);
	if (off >= len) {
		error = uiomove(buf, 0, uio);
	} else {
		error = uiomove(&buf[off], len - off, uio);
	}
	return error;
}

static struct pciback_pci_dev*
pciback_pci_lookup(u_int bus, u_int dev, u_int func)
{
	struct pciback_pci_dev *pbd;
	SLIST_FOREACH(pbd, &pciback_pci_devlist_head, pb_devlist_next) {
		if (pbd->pb_bus == bus &&
		    pbd->pb_device == dev &&
		    pbd->pb_function == func)
			return pbd;
	}
	return NULL;
}

static void
pciback_pci_init(void)
{
	union xen_cmdline_parseinfo xi;
	char *pcidevs, *c;
	u_int bus, dev, func;
	struct pciback_pci_dev *pb;
	kernfs_entry_t *dkt;

	memset(&xi, 0, sizeof(xi));
	xen_parse_cmdline(XEN_PARSE_PCIBACK, &xi);
	if (strlen(xi.xcp_pcidevs) == 0)
		return;
	pcidevs = xi.xcp_pcidevs;
	for(pcidevs = xi.xcp_pcidevs; *pcidevs != '\0';) {
		if (*pcidevs != '(')
			goto error;
		pcidevs++;
		/* parse location */
		c = strchr(pcidevs, ')');
		if (c == NULL)
			goto error;
		*c = '\0';
		if (pciback_parse_pci(pcidevs, &bus, &dev, &func) == 0) {
			pb = malloc(sizeof(struct pciback_pci_dev), M_DEVBUF,
			    M_NOWAIT | M_ZERO);
			if (pb == NULL) {
				aprint_error("pciback_pci_init: out or memory\n");
				return;
			}
			pb->pb_bus = bus;
			pb->pb_device = dev;
			pb->pb_function = func;
			aprint_verbose("pciback_pci_init: hide claim device "
			    "%x:%x:%x\n", bus, dev, func);
			SLIST_INSERT_HEAD(&pciback_pci_devlist_head, pb,
			    pb_devlist_next);
		}
		pcidevs = c + 1;
	}
	xenbus_backend_register(&pci_backend_driver);

	KERNFS_ALLOCENTRY(dkt, M_TEMP, M_WAITOK);
	KERNFS_INITENTRY(dkt, DT_DIR, "pci", NULL, KFSsubdir, VDIR, DIR_MODE);
	kernfs_addentry(kernxen_pkt, dkt);
	pciback_kern_pkt = KERNFS_ENTOPARENTDIR(dkt);
	return;
error:
	aprint_error("pciback_pci_init: syntax error at %s\n", pcidevs);
	return;
}

static int
pciback_parse_pci(const char *str, u_int *busp, u_int *devp, u_int *funcp)
{
	char *c;

	/* parse location */
	c = strchr(str, ':');
	if (c == NULL)
		goto error;
	if (strncmp("0000", str, 4) == 0) {
		/* must be domain number, get next */
		str = c + 1;
	}
	*busp = strtoul(str, &c, 16);
	if (*c != ':')
		goto error;
	str = c + 1;
	*devp = strtoul(str, &c, 16);
	if (*c != '.')
		goto error;
	str = c + 1;
	*funcp = strtoul(str, &c, 16);
	if (*c != '\0')
		goto error;
	return 0;
error:
	aprint_error("pciback_pci_init: syntax error at char %c\n", *c);
	return EINVAL;
}

static int
pciback_xenbus_create(struct xenbus_device *xbusd)
{
	struct pb_xenbus_instance *pbxi;
	long domid;
	char *val;
	char path[10];
	int i, err;
	u_long num_devs;

	if ((err = xenbus_read_ul(NULL, xbusd->xbusd_path,
	    "frontend-id", &domid, 10)) != 0) {
		aprint_error("pciback: can' read %s/frontend-id: %d\n",
		    xbusd->xbusd_path, err);
		return err;
	}

	if (pbxif_lookup(domid) != NULL) {
		return EEXIST;
	}
	pbxi = malloc(sizeof(struct pb_xenbus_instance), M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	if (pbxi == NULL) {
		return ENOMEM;
	}
	pbxi->pbx_domid = domid;

	xbusd->xbusd_u.b.b_cookie = pbxi;
	xbusd->xbusd_u.b.b_detach = pciback_xenbus_destroy;
	pbxi->pbx_xbusd = xbusd;

	SLIST_INIT(&pbxi->pbx_pb_pci_dev);

	SLIST_INSERT_HEAD(&pb_xenbus_instances, pbxi, pbx_next);

	xbusd->xbusd_otherend_changed = pciback_xenbus_frontend_changed;

	err = xenbus_switch_state(xbusd, NULL, XenbusStateInitWait);
	if (err) {
		printf("failed to switch state on %s: %d\n",
		    xbusd->xbusd_path, err);
		goto fail;
	}
	if ((err = xenbus_read_ul(NULL, xbusd->xbusd_path, "num_devs",
	    &num_devs, 10)) != 0) {
		aprint_error("pciback: can' read %s/num_devs %d\n",
		    xbusd->xbusd_path, err);
		goto fail;
	}
	for (i = 0; i < num_devs; i++) {
		snprintf(path, sizeof(path), "dev-%d", i);
		if ((err = xenbus_read(NULL, xbusd->xbusd_path, path,
		    NULL, &val)) != 0) {
			aprint_error("pciback: can' read %s/%s: %d\n",
			    xbusd->xbusd_path, path, err);
			goto fail;
		}
		pciback_xenbus_export_device(pbxi, val);
		free(val, M_DEVBUF);
	}
	pciback_xenbus_export_roots(pbxi);
	if ((err = xenbus_switch_state(xbusd, NULL, XenbusStateInitialised))) {
		printf("failed to switch state on %s: %d\n",
		    xbusd->xbusd_path, err);
		goto fail;
	}

	return 0;
fail:
	free(pbxi, M_DEVBUF);
	return err;
}

static int
pciback_xenbus_destroy(void *arg)
{
	struct pb_xenbus_instance *pbxi = arg;
	struct pciback_pci_dev *pbd; 
	struct gnttab_unmap_grant_ref op;
	int err;

	hypervisor_mask_event(pbxi->pbx_evtchn);
	event_remove_handler(pbxi->pbx_evtchn,
	    pciback_xenbus_evthandler, pbxi);

	SLIST_REMOVE(&pb_xenbus_instances,
	    pbxi, pb_xenbus_instance, pbx_next);

	if (pbxi->pbx_sh_info) {
		op.host_addr = (vaddr_t)pbxi->pbx_sh_info;
		op.handle = pbxi->pbx_shinfo_handle;
		op.dev_bus_addr = 0;
		err = HYPERVISOR_grant_table_op(GNTTABOP_unmap_grant_ref,
		    &op, 1);
		if (err)
			aprint_error("pciback: unmap_grant_ref failed: %d\n",
			    err);
	}
	SLIST_FOREACH(pbd, &pbxi->pbx_pb_pci_dev, pb_guest_next) {
		pbd->pbx_instance = NULL;
	}
	uvm_km_free(kernel_map, (vaddr_t)pbxi->pbx_sh_info,
	    PAGE_SIZE, UVM_KMF_VAONLY);
	free(pbxi, M_DEVBUF);
	return 0;
}

static void
pciback_xenbus_frontend_changed(void *arg, XenbusState new_state)
{
	struct pb_xenbus_instance *pbxi = arg;
	struct xenbus_device *xbusd = pbxi->pbx_xbusd;
	int err;
	struct gnttab_map_grant_ref op;
	evtchn_op_t evop;
	u_long shared_ref;
	u_long revtchn;

	/* do it only once */
	if (xenbus_read_driver_state(xbusd->xbusd_path) !=
	    XenbusStateInitialised)
		return;
		
	switch(new_state) {
	case XenbusStateInitialising:
	case XenbusStateConnected:
		break;
	case XenbusStateInitialised:
		/* read comunication informations */
		err = xenbus_read_ul(NULL, xbusd->xbusd_otherend,
		    "pci-op-ref", &shared_ref, 10);
		if (err) {
			xenbus_dev_fatal(xbusd, err, "reading %s/pci-op-ref",
			    xbusd->xbusd_otherend);
			break;
		}
		err = xenbus_read_ul(NULL, xbusd->xbusd_otherend,
		    "event-channel", &revtchn, 10);
		if (err) {
			xenbus_dev_fatal(xbusd, err, "reading %s/event-channel",
			    xbusd->xbusd_otherend);
			break;
		}
		/* allocate VA space and map rings */
		pbxi->pbx_sh_info = (void *)uvm_km_alloc(kernel_map,
		    PAGE_SIZE, 0, UVM_KMF_VAONLY);
		if (pbxi->pbx_sh_info == 0) {
			xenbus_dev_fatal(xbusd, ENOMEM,
			    "can't get VA for shared infos",
			    xbusd->xbusd_otherend);
			break;
		}
		op.host_addr = (vaddr_t)pbxi->pbx_sh_info;
		op.flags = GNTMAP_host_map;
		op.ref = shared_ref;
		op.dom = pbxi->pbx_domid;
		err = HYPERVISOR_grant_table_op(GNTTABOP_map_grant_ref, &op, 1);
		if (err || op.status) {
			aprint_error("pciback: can't map shared grant ref: "
			    "%d/%d\n", err, op.status);
			goto err1;
		}
		pbxi->pbx_shinfo_handle = op.handle;

		evop.cmd = EVTCHNOP_bind_interdomain;
		evop.u.bind_interdomain.remote_dom = pbxi->pbx_domid;
		evop.u.bind_interdomain.remote_port = revtchn;
		err = HYPERVISOR_event_channel_op(&evop);
		if (err) {
			printf("pciback: can't get event channel: %d\n", err);
			goto err1;
		}
		pbxi->pbx_evtchn = evop.u.bind_interdomain.local_port;
		x86_sfence();
		xenbus_switch_state(xbusd, NULL, XenbusStateConnected);
		x86_sfence();
		event_set_handler(pbxi->pbx_evtchn, pciback_xenbus_evthandler,
		    pbxi, IPL_BIO, "pciback");
		hypervisor_enable_event(pbxi->pbx_evtchn);
		hypervisor_notify_via_evtchn(pbxi->pbx_evtchn);
		break;

	case XenbusStateClosing:
		xenbus_switch_state(xbusd, NULL, XenbusStateClosing);
		break;

	case XenbusStateClosed:
		/* otherend_changed() should handle it for us */
		panic("pciback_xenbus_frontend_changed: closed\n");
	case XenbusStateUnknown:      
	case XenbusStateInitWait:
	default:
		aprint_error("pciback: invalid frontend state %d\n",
		    new_state);
		break;
	}
	return;
err1:
	uvm_km_free(kernel_map, (vaddr_t)pbxi->pbx_sh_info,
	    PAGE_SIZE, UVM_KMF_VAONLY);
}

/* lookup a pbxi based on domain id and interface handle */
static struct pb_xenbus_instance *
pbxif_lookup(domid_t dom)
{
	struct pb_xenbus_instance *pbxi;

	SLIST_FOREACH(pbxi, &pb_xenbus_instances, pbx_next) {
		if (pbxi->pbx_domid == dom)
			return pbxi;
	}
	return NULL;
}

static void
pciback_xenbus_export_device(struct pb_xenbus_instance *pbxi, char *val)
{
	u_int bus, dev, func;
	struct pciback_pci_dev *pbd;
	if (pciback_parse_pci(val, &bus, &dev, &func)) {
		aprint_error("pciback: can't parse %s\n", val);
		return;
	}
	pbd = pciback_pci_lookup(bus, dev, func);
	if (pbd == NULL) {
		aprint_error("pciback: can't locate 0x%02x:0x%02x.0x%02x\n",
		    bus, dev, func);
		return;
	}
	if (pbd->pb_pci_softc == NULL) {
		aprint_error("pciback: 0x%02x:0x%02x.0x%02x not detected\n",
		    bus, dev, func);
		return;
	}
	pbd->pbx_instance = pbxi;
	SLIST_INSERT_HEAD(&pbxi->pbx_pb_pci_dev, pbd, pb_guest_next);
	return;
}

static void
pciback_xenbus_export_roots(struct pb_xenbus_instance *pbxi)
{
	char bus[256];
	char root[10];
	struct pciback_pci_dev *pbd;
	int num_roots = 0;
	int err;

	memset(bus, 0, sizeof(bus));

	SLIST_FOREACH(pbd, &pbxi->pbx_pb_pci_dev, pb_guest_next) {
		if (bus[pbd->pb_bus] == 0) {
			/* not published yet */
			snprintf(root, sizeof(root), "root-%d", num_roots);
			err = xenbus_printf(NULL,
			    pbxi->pbx_xbusd->xbusd_path, 
			    root, "0000:%02x", pbd->pb_bus);
			if (err) {
				aprint_error("pciback: can't write to %s/%s: "
				    "%d\n", pbxi->pbx_xbusd->xbusd_path, root,
				    err);
			}
			num_roots++;
		}
	}
	err = xenbus_printf(NULL, pbxi->pbx_xbusd->xbusd_path, "root_num",
	    "%d", num_roots);
	if (err) {
		aprint_error("pciback: can't write to %s/root_num: "
		    "%d\n", pbxi->pbx_xbusd->xbusd_path, err);
	}
}

static int
pciback_xenbus_evthandler(void * arg)
{
	struct pb_xenbus_instance *pbxi = arg;
	struct pciback_pci_dev *pbd;
	struct xen_pci_op *op = &pbxi->pbx_sh_info->op;
	u_int bus, dev, func;

	hypervisor_clear_event(pbxi->pbx_evtchn);
	if (xen_atomic_test_bit(&pbxi->pbx_sh_info->flags,
	    _XEN_PCIF_active) == 0)
		return 0;
	if (op->domain != 0) {
		aprint_error("pciback: domain %d != 0", op->domain);
		op->err = XEN_PCI_ERR_dev_not_found;
		goto end;
	}
	bus = op->bus;
	dev = (op->devfn >> 3) & 0xff;
	func = (op->devfn) & 0x7;
	SLIST_FOREACH(pbd, &pbxi->pbx_pb_pci_dev, pb_guest_next) {
		if (pbd->pb_bus == bus &&
		    pbd->pb_device == dev &&
		    pbd->pb_function == func)
			break;
	}
	if (pbd == NULL) {
		aprint_error("pciback: %02x:%02x.%x not found\n",
		    bus, dev, func);
		op->err = XEN_PCI_ERR_dev_not_found;
		goto end;
	}
	switch(op->cmd) {
	case XEN_PCI_OP_conf_read:
		op->err = XEN_PCI_ERR_success;
		switch (op->size) {
		case 1:
			op->value = pciback_read_byte(pbd->pb_pc, pbd->pb_tag,
			    op->offset);
			break;
		case 2:
			op->value = pciback_read_word(pbd->pb_pc, pbd->pb_tag,
			    op->offset);
			break;
		case 4:
			op->value = pci_conf_read(pbd->pb_pc, pbd->pb_tag,
			    op->offset);
			break;
		default:
			aprint_error("pciback: bad size %d\n", op->size);
			break;
		}
		break;
	case XEN_PCI_OP_conf_write:
		op->err = XEN_PCI_ERR_success;
		switch(op->size) {
		case 1:
			pciback_write_byte(pbd->pb_pc, pbd->pb_tag,
			    op->offset, op->value);
			break;
		case 2:
			pciback_write_word(pbd->pb_pc, pbd->pb_tag,
			    op->offset, op->value);
			break;
		case 4:
			pci_conf_write(pbd->pb_pc, pbd->pb_tag,
			    op->offset, op->value);
			break;
		default:
			aprint_error("pciback: bad size %d\n", op->size);
			op->err = XEN_PCI_ERR_invalid_offset;
			break;
		}
	default:
		aprint_error("pciback: unknown cmd %d\n", op->cmd);
		op->err = XEN_PCI_ERR_not_implemented;
	}
end:
	xen_atomic_clear_bit(&pbxi->pbx_sh_info->flags, _XEN_PCIF_active);
	hypervisor_notify_via_evtchn(pbxi->pbx_evtchn);
	return 1;
}
