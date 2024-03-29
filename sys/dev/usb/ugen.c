/*	$NetBSD: ugen.c,v 1.177 2024/03/29 19:30:09 thorpej Exp $	*/

/*
 * Copyright (c) 1998, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Copyright (c) 2006 BBN Technologies Corp.  All rights reserved.
 * Effort sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and the Department of the Interior National Business
 * Center under agreement number NBCHC050166.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ugen.c,v 1.177 2024/03/29 19:30:09 thorpej Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/poll.h>
#include <sys/compat_stub.h>
#include <sys/module.h>
#include <sys/rbtree.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbhist.h>

#include "ioconf.h"

#ifdef USB_DEBUG
#ifndef UGEN_DEBUG
#define ugendebug 0
#else

#ifndef UGEN_DEBUG_DEFAULT
#define UGEN_DEBUG_DEFAULT 0
#endif

int	ugendebug = UGEN_DEBUG_DEFAULT;

SYSCTL_SETUP(sysctl_hw_ugen_setup, "sysctl hw.ugen setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "ugen",
	    SYSCTL_DESCR("ugen global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &ugendebug, sizeof(ugendebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* UGEN_DEBUG */
#endif /* USB_DEBUG */

#define DPRINTF(FMT,A,B,C,D)    USBHIST_LOGN(ugendebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D) USBHIST_LOGN(ugendebug,N,FMT,A,B,C,D)
#define UGENHIST_FUNC()         USBHIST_FUNC()
#define UGENHIST_CALLED(name)   USBHIST_CALLED(ugendebug)
#define UGENHIST_CALLARGS(FMT,A,B,C,D) \
				USBHIST_CALLARGS(ugendebug,FMT,A,B,C,D)
#define UGENHIST_CALLARGSN(N,FMT,A,B,C,D) \
				USBHIST_CALLARGSN(ugendebug,N,FMT,A,B,C,D)

#define	UGEN_CHUNK	128	/* chunk size for read */
#define	UGEN_IBSIZE	1020	/* buffer size */
#define	UGEN_BBSIZE	1024

#define UGEN_NISOREQS	4	/* number of outstanding xfer requests */
#define UGEN_NISORFRMS	8	/* number of transactions per req */
#define UGEN_NISOFRAMES	(UGEN_NISORFRMS * UGEN_NISOREQS)

#define UGEN_BULK_RA_WB_BUFSIZE	16384		/* default buffer size */
#define UGEN_BULK_RA_WB_BUFMAX	(1 << 20)	/* maximum allowed buffer */

struct isoreq {
	struct ugen_endpoint *sce;
	struct usbd_xfer *xfer;
	void *dmabuf;
	uint16_t sizes[UGEN_NISORFRMS];
};

struct ugen_endpoint {
	struct ugen_softc *sc;
	usb_endpoint_descriptor_t *edesc;
	struct usbd_interface *iface;
	int state;
#define UGEN_SHORT_OK	0x04	/* short xfers are OK */
#define UGEN_BULK_RA	0x08	/* in bulk read-ahead mode */
#define UGEN_BULK_WB	0x10	/* in bulk write-behind mode */
#define UGEN_RA_WB_STOP	0x20	/* RA/WB xfer is stopped (buffer full/empty) */
	struct usbd_pipe *pipeh;
	struct clist q;
	u_char *ibuf;		/* start of buffer (circular for isoc) */
	u_char *fill;		/* location for input (isoc) */
	u_char *limit;		/* end of circular buffer (isoc) */
	u_char *cur;		/* current read location (isoc) */
	uint32_t timeout;
	uint32_t ra_wb_bufsize; /* requested size for RA/WB buffer */
	uint32_t ra_wb_reqsize; /* requested xfer length for RA/WB */
	uint32_t ra_wb_used;	 /* how much is in buffer */
	uint32_t ra_wb_xferlen; /* current xfer length for RA/WB */
	struct usbd_xfer *ra_wb_xfer;
	struct isoreq isoreqs[UGEN_NISOREQS];
	/* Keep these last; we don't overwrite them in ugen_set_config() */
#define UGEN_ENDPOINT_NONZERO_CRUFT	offsetof(struct ugen_endpoint, rsel)
	struct selinfo rsel;
	kcondvar_t cv;
};

struct ugen_softc {
	device_t sc_dev;		/* base device */
	struct usbd_device *sc_udev;
	struct rb_node sc_node;
	unsigned sc_unit;

	kmutex_t		sc_lock;
	kcondvar_t		sc_detach_cv;

	char sc_is_open[USB_MAX_ENDPOINTS];
	struct ugen_endpoint sc_endpoints[USB_MAX_ENDPOINTS][2];
#define OUT 0
#define IN  1

	int sc_refcnt;
	char sc_buffer[UGEN_BBSIZE];
	u_char sc_dying;
	u_char sc_attached;
};

static struct {
	kmutex_t	lock;
	rb_tree_t	tree;
} ugenif __cacheline_aligned;

static int
compare_ugen(void *cookie, const void *vsca, const void *vscb)
{
	const struct ugen_softc *sca = vsca;
	const struct ugen_softc *scb = vscb;

	if (sca->sc_unit < scb->sc_unit)
		return -1;
	if (sca->sc_unit > scb->sc_unit)
		return +1;
	return 0;
}

static int
compare_ugen_key(void *cookie, const void *vsc, const void *vk)
{
	const struct ugen_softc *sc = vsc;
	const unsigned *k = vk;

	if (sc->sc_unit < *k)
		return -1;
	if (sc->sc_unit > *k)
		return +1;
	return 0;
}

static const rb_tree_ops_t ugenif_tree_ops = {
	.rbto_compare_nodes = compare_ugen,
	.rbto_compare_key = compare_ugen_key,
	.rbto_node_offset = offsetof(struct ugen_softc, sc_node),
};

static void
ugenif_get_unit(struct ugen_softc *sc)
{
	struct ugen_softc *sc0;
	unsigned i;

	mutex_enter(&ugenif.lock);
	for (i = 0, sc0 = RB_TREE_MIN(&ugenif.tree);
	     sc0 != NULL && i == sc0->sc_unit;
	     i++, sc0 = RB_TREE_NEXT(&ugenif.tree, sc0))
		KASSERT(i < UINT_MAX);
	KASSERT(rb_tree_find_node(&ugenif.tree, &i) == NULL);
	sc->sc_unit = i;
	sc0 = rb_tree_insert_node(&ugenif.tree, sc);
	KASSERT(sc0 == sc);
	KASSERT(rb_tree_find_node(&ugenif.tree, &i) == sc);
	mutex_exit(&ugenif.lock);

	prop_dictionary_set_uint(device_properties(sc->sc_dev),
	    "ugen-unit", sc->sc_unit);
}

static void
ugenif_put_unit(struct ugen_softc *sc)
{

	prop_dictionary_remove(device_properties(sc->sc_dev),
	    "ugen-unit");

	mutex_enter(&ugenif.lock);
	KASSERT(rb_tree_find_node(&ugenif.tree, &sc->sc_unit) == sc);
	rb_tree_remove_node(&ugenif.tree, sc);
	sc->sc_unit = -1;
	mutex_exit(&ugenif.lock);
}

static struct ugen_softc *
ugenif_acquire(unsigned unit)
{
	struct ugen_softc *sc;

	mutex_enter(&ugenif.lock);
	sc = rb_tree_find_node(&ugenif.tree, &unit);
	if (sc == NULL)
		goto out;
	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(&sc->sc_lock);
		sc = NULL;
		goto out;
	}
	KASSERT(sc->sc_refcnt < INT_MAX);
	sc->sc_refcnt++;
	mutex_exit(&sc->sc_lock);
out:	mutex_exit(&ugenif.lock);

	return sc;
}

static void
ugenif_release(struct ugen_softc *sc)
{

	mutex_enter(&sc->sc_lock);
	if (--sc->sc_refcnt < 0)
		cv_broadcast(&sc->sc_detach_cv);
	mutex_exit(&sc->sc_lock);
}

static dev_type_open(ugenopen);
static dev_type_close(ugenclose);
static dev_type_read(ugenread);
static dev_type_write(ugenwrite);
static dev_type_ioctl(ugenioctl);
static dev_type_poll(ugenpoll);
static dev_type_kqfilter(ugenkqfilter);

const struct cdevsw ugen_cdevsw = {
	.d_open = ugenopen,
	.d_close = ugenclose,
	.d_read = ugenread,
	.d_write = ugenwrite,
	.d_ioctl = ugenioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = ugenpoll,
	.d_mmap = nommap,
	.d_kqfilter = ugenkqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};

Static void ugenintr(struct usbd_xfer *, void *,
		     usbd_status);
Static void ugen_isoc_rintr(struct usbd_xfer *, void *,
			    usbd_status);
Static void ugen_bulkra_intr(struct usbd_xfer *, void *,
			     usbd_status);
Static void ugen_bulkwb_intr(struct usbd_xfer *, void *,
			     usbd_status);
Static int ugen_do_read(struct ugen_softc *, int, struct uio *, int);
Static int ugen_do_write(struct ugen_softc *, int, struct uio *, int);
Static int ugen_do_ioctl(struct ugen_softc *, int, u_long,
			 void *, int, struct lwp *);
Static int ugen_set_config(struct ugen_softc *, int, int);
Static usb_config_descriptor_t *ugen_get_cdesc(struct ugen_softc *,
					       int, int *);
Static usbd_status ugen_set_interface(struct ugen_softc *, int, int);
Static int ugen_get_alt_index(struct ugen_softc *, int);
Static void ugen_clear_endpoints(struct ugen_softc *);

#define UGENUNIT(n) ((minor(n) >> 4) & 0xf)
#define UGENENDPOINT(n) (minor(n) & 0xf)
#define UGENDEV(u, e) (makedev(0, ((u) << 4) | (e)))

static int	ugenif_match(device_t, cfdata_t, void *);
static void	ugenif_attach(device_t, device_t, void *);
static int	ugen_match(device_t, cfdata_t, void *);
static void	ugen_attach(device_t, device_t, void *);
static int	ugen_detach(device_t, int);
static int	ugen_activate(device_t, enum devact);

CFATTACH_DECL_NEW(ugen, sizeof(struct ugen_softc), ugen_match,
    ugen_attach, ugen_detach, ugen_activate);
CFATTACH_DECL_NEW(ugenif, sizeof(struct ugen_softc), ugenif_match,
    ugenif_attach, ugen_detach, ugen_activate);

/* toggle to control attach priority. -1 means "let autoconf decide" */
int ugen_override = -1;

static int
ugen_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;
	int override;

	if (ugen_override != -1)
		override = ugen_override;
	else
		override = match->cf_flags & 1;

	if (override)
		return UMATCH_HIGHEST;
	else if (uaa->uaa_usegeneric)
		return UMATCH_GENERIC;
	else
		return UMATCH_NONE;
}

static int
ugenif_match(device_t parent, cfdata_t match, void *aux)
{
	/*
	 * Like ugen(4), ugenif(4) also has an override flag.  It has the
	 * opposite effect, however, causing us to match with GENERIC
	 * priority rather than HIGHEST.
	 */
	return (match->cf_flags & 1) ? UMATCH_GENERIC : UMATCH_HIGHEST;
}

static void
ugen_attach(device_t parent, device_t self, void *aux)
{
	struct usb_attach_arg *uaa = aux;
	struct usbif_attach_arg uiaa;

	memset(&uiaa, 0, sizeof(uiaa));
	uiaa.uiaa_port = uaa->uaa_port;
	uiaa.uiaa_vendor = uaa->uaa_vendor;
	uiaa.uiaa_product = uaa->uaa_product;
	uiaa.uiaa_release = uaa->uaa_release;
	uiaa.uiaa_device = uaa->uaa_device;
	uiaa.uiaa_configno = -1;
	uiaa.uiaa_ifaceno = -1;

	ugenif_attach(parent, self, &uiaa);
}

static void
ugenif_attach(device_t parent, device_t self, void *aux)
{
	struct ugen_softc *sc = device_private(self);
	struct usbif_attach_arg *uiaa = aux;
	struct usbd_device *udev;
	char *devinfop;
	usbd_status err;
	int i, dir, conf;

	aprint_naive("\n");
	aprint_normal("\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	cv_init(&sc->sc_detach_cv, "ugendet");

	devinfop = usbd_devinfo_alloc(uiaa->uiaa_device, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_dev = self;
	sc->sc_udev = udev = uiaa->uiaa_device;

	for (i = 0; i < USB_MAX_ENDPOINTS; i++) {
		for (dir = OUT; dir <= IN; dir++) {
			struct ugen_endpoint *sce;

			sce = &sc->sc_endpoints[i][dir];
			selinit(&sce->rsel);
			cv_init(&sce->cv, "ugensce");
		}
	}

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	if (uiaa->uiaa_ifaceno < 0) {
		/*
		 * If we attach the whole device,
		 * set configuration index 0, the default one.
		 */
		err = usbd_set_config_index(udev, 0, 0);
		if (err) {
			aprint_error_dev(self,
			    "setting configuration index 0 failed\n");
			return;
		}
	}

	/* Get current configuration */
	conf = usbd_get_config_descriptor(udev)->bConfigurationValue;

	/* Set up all the local state for this configuration. */
	err = ugen_set_config(sc, conf, uiaa->uiaa_ifaceno < 0);
	if (err) {
		aprint_error_dev(self, "setting configuration %d failed\n",
		    conf);
		return;
	}

	ugenif_get_unit(sc);
	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);
	sc->sc_attached = 1;
}

Static void
ugen_clear_endpoints(struct ugen_softc *sc)
{

	/* Clear out the old info, but leave the selinfo and cv initialised. */
	for (int i = 0; i < USB_MAX_ENDPOINTS; i++) {
		for (int dir = OUT; dir <= IN; dir++) {
			struct ugen_endpoint *sce = &sc->sc_endpoints[i][dir];
			memset(sce, 0, UGEN_ENDPOINT_NONZERO_CRUFT);
		}
	}
}

Static int
ugen_set_config(struct ugen_softc *sc, int configno, int chkopen)
{
	struct usbd_device *dev = sc->sc_udev;
	usb_config_descriptor_t *cdesc;
	struct usbd_interface *iface;
	usb_endpoint_descriptor_t *ed;
	struct ugen_endpoint *sce;
	uint8_t niface, nendpt;
	int ifaceno, endptno, endpt;
	usbd_status err;
	int dir;

	UGENHIST_FUNC();
	UGENHIST_CALLARGSN(1, "ugen%jd: to configno %jd, sc=%jx",
	    device_unit(sc->sc_dev), configno, (uintptr_t)sc, 0);

	KASSERT(KERNEL_LOCKED_P()); /* sc_is_open */

	if (chkopen) {
		/*
		 * We start at 1, not 0, because we don't care whether the
		 * control endpoint is open or not. It is always present.
		 */
		for (endptno = 1; endptno < USB_MAX_ENDPOINTS; endptno++)
			if (sc->sc_is_open[endptno]) {
				DPRINTFN(1,
				     "ugen%jd - endpoint %d is open",
				      device_unit(sc->sc_dev), endptno, 0, 0);
				return USBD_IN_USE;
			}

		/* Prevent opening while we're setting the config.  */
		for (endptno = 1; endptno < USB_MAX_ENDPOINTS; endptno++) {
			KASSERT(!sc->sc_is_open[endptno]);
			sc->sc_is_open[endptno] = 1;
		}
	}

	/* Avoid setting the current value. */
	cdesc = usbd_get_config_descriptor(dev);
	if (!cdesc || cdesc->bConfigurationValue != configno) {
		err = usbd_set_config_no(dev, configno, 1);
		if (err)
			goto out;
	}

	ugen_clear_endpoints(sc);

	err = usbd_interface_count(dev, &niface);
	if (err)
		goto out;

	for (ifaceno = 0; ifaceno < niface; ifaceno++) {
		DPRINTFN(1, "ifaceno %jd", ifaceno, 0, 0, 0);
		err = usbd_device2interface_handle(dev, ifaceno, &iface);
		if (err)
			goto out;
		err = usbd_endpoint_count(iface, &nendpt);
		if (err)
			goto out;
		for (endptno = 0; endptno < nendpt; endptno++) {
			ed = usbd_interface2endpoint_descriptor(iface, endptno);
			KASSERT(ed != NULL);
			endpt = ed->bEndpointAddress;
			dir = UE_GET_DIR(endpt) == UE_DIR_IN ? IN : OUT;
			sce = &sc->sc_endpoints[UE_GET_ADDR(endpt)][dir];
			DPRINTFN(1, "endptno %jd, endpt=0x%02jx (%jd,%jd)",
				 endptno, endpt, UE_GET_ADDR(endpt),
				 UE_GET_DIR(endpt));
			sce->sc = sc;
			sce->edesc = ed;
			sce->iface = iface;
		}
	}
	err = USBD_NORMAL_COMPLETION;

out:	if (chkopen) {
		/*
		 * Allow open again now that we're done trying to set
		 * the config.
		 */
		for (endptno = 1; endptno < USB_MAX_ENDPOINTS; endptno++) {
			KASSERT(sc->sc_is_open[endptno]);
			sc->sc_is_open[endptno] = 0;
		}
	}
	return err;
}

static int
ugenopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ugen_softc *sc;
	int unit = UGENUNIT(dev);
	int endpt = UGENENDPOINT(dev);
	usb_endpoint_descriptor_t *edesc;
	struct ugen_endpoint *sce;
	int dir, isize;
	usbd_status err;
	struct usbd_xfer *xfer;
	int i, j;
	int error;
	int opened = 0;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("flag=%jd, mode=%jd, unit=%jd endpt=%jd",
	    flag, mode, unit, endpt);

	KASSERT(KERNEL_LOCKED_P()); /* sc_is_open */

	if ((sc = ugenif_acquire(unit)) == NULL)
		return ENXIO;

	/* The control endpoint allows multiple opens. */
	if (endpt == USB_CONTROL_ENDPOINT) {
		opened = sc->sc_is_open[USB_CONTROL_ENDPOINT] = 1;
		error = 0;
		goto out;
	}

	if (sc->sc_is_open[endpt]) {
		error = EBUSY;
		goto out;
	}
	opened = sc->sc_is_open[endpt] = 1;

	/* Make sure there are pipes for all directions. */
	for (dir = OUT; dir <= IN; dir++) {
		if (flag & (dir == OUT ? FWRITE : FREAD)) {
			sce = &sc->sc_endpoints[endpt][dir];
			if (sce->edesc == NULL) {
				error = ENXIO;
				goto out;
			}
		}
	}

	/* Actually open the pipes. */
	/* XXX Should back out properly if it fails. */
	for (dir = OUT; dir <= IN; dir++) {
		if (!(flag & (dir == OUT ? FWRITE : FREAD)))
			continue;
		sce = &sc->sc_endpoints[endpt][dir];
		sce->state = 0;
		sce->timeout = USBD_NO_TIMEOUT;
		DPRINTFN(5, "sc=%jx, endpt=%jd, dir=%jd, sce=%jp",
			     (uintptr_t)sc, endpt, dir, (uintptr_t)sce);
		edesc = sce->edesc;
		switch (edesc->bmAttributes & UE_XFERTYPE) {
		case UE_INTERRUPT:
			if (dir == OUT) {
				err = usbd_open_pipe(sce->iface,
				    edesc->bEndpointAddress, 0, &sce->pipeh);
				if (err) {
					error = EIO;
					goto out;
				}
				break;
			}
			isize = UGETW(edesc->wMaxPacketSize);
			if (isize == 0) {	/* shouldn't happen */
				error = EINVAL;
				goto out;
			}
			sce->ibuf = kmem_alloc(isize, KM_SLEEP);
			DPRINTFN(5, "intr endpt=%d, isize=%d",
				     endpt, isize, 0, 0);
			if (clalloc(&sce->q, UGEN_IBSIZE, 0) == -1) {
				kmem_free(sce->ibuf, isize);
				sce->ibuf = NULL;
				error = ENOMEM;
				goto out;
			}
			err = usbd_open_pipe_intr(sce->iface,
				  edesc->bEndpointAddress,
				  USBD_SHORT_XFER_OK, &sce->pipeh, sce,
				  sce->ibuf, isize, ugenintr,
				  USBD_DEFAULT_INTERVAL);
			if (err) {
				clfree(&sce->q);
				kmem_free(sce->ibuf, isize);
				sce->ibuf = NULL;
				error = EIO;
				goto out;
			}
			DPRINTFN(5, "interrupt open done", 0, 0, 0, 0);
			break;
		case UE_BULK:
			err = usbd_open_pipe(sce->iface,
				  edesc->bEndpointAddress, 0, &sce->pipeh);
			if (err) {
				error = EIO;
				goto out;
			}
			sce->ra_wb_bufsize = UGEN_BULK_RA_WB_BUFSIZE;
			/*
			 * Use request size for non-RA/WB transfers
			 * as the default.
			 */
			sce->ra_wb_reqsize = UGEN_BBSIZE;
			break;
		case UE_ISOCHRONOUS:
			if (dir == OUT) {
				error = EINVAL;
				goto out;
			}
			isize = UGETW(edesc->wMaxPacketSize);
			if (isize == 0) {	/* shouldn't happen */
				error = EINVAL;
				goto out;
			}
			sce->ibuf = kmem_alloc(isize * UGEN_NISOFRAMES,
				KM_SLEEP);
			sce->cur = sce->fill = sce->ibuf;
			sce->limit = sce->ibuf + isize * UGEN_NISOFRAMES;
			DPRINTFN(5, "isoc endpt=%d, isize=%d",
				     endpt, isize, 0, 0);
			err = usbd_open_pipe(sce->iface,
				  edesc->bEndpointAddress, 0, &sce->pipeh);
			if (err) {
				kmem_free(sce->ibuf, isize * UGEN_NISOFRAMES);
				sce->ibuf = NULL;
				error = EIO;
				goto out;
			}
			for (i = 0; i < UGEN_NISOREQS; ++i) {
				sce->isoreqs[i].sce = sce;
				err = usbd_create_xfer(sce->pipeh,
				    isize * UGEN_NISORFRMS, 0, UGEN_NISORFRMS,
				    &xfer);
				if (err)
					goto bad;
				sce->isoreqs[i].xfer = xfer;
				sce->isoreqs[i].dmabuf = usbd_get_buffer(xfer);
				for (j = 0; j < UGEN_NISORFRMS; ++j)
					sce->isoreqs[i].sizes[j] = isize;
				usbd_setup_isoc_xfer(xfer, &sce->isoreqs[i],
				    sce->isoreqs[i].sizes, UGEN_NISORFRMS, 0,
				    ugen_isoc_rintr);
				(void)usbd_transfer(xfer);
			}
			DPRINTFN(5, "isoc open done", 0, 0, 0, 0);
			break;
		bad:
			while (--i >= 0) { /* implicit buffer free */
				usbd_destroy_xfer(sce->isoreqs[i].xfer);
				sce->isoreqs[i].xfer = NULL;
			}
			usbd_close_pipe(sce->pipeh);
			sce->pipeh = NULL;
			kmem_free(sce->ibuf, isize * UGEN_NISOFRAMES);
			sce->ibuf = NULL;
			error = ENOMEM;
			goto out;
		case UE_CONTROL:
			sce->timeout = USBD_DEFAULT_TIMEOUT;
			error = EINVAL;
			goto out;
		}
	}
	error = 0;
out:	if (error && opened)
		sc->sc_is_open[endpt] = 0;
	ugenif_release(sc);
	return error;
}

static void
ugen_do_close(struct ugen_softc *sc, int flag, int endpt)
{
	struct ugen_endpoint *sce;
	int dir;
	int i;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("flag=%jd endpt=%jd", flag, endpt, 0, 0);

	KASSERT(KERNEL_LOCKED_P()); /* sc_is_open */

	if (!sc->sc_is_open[endpt])
		goto out;

	if (endpt == USB_CONTROL_ENDPOINT) {
		DPRINTFN(5, "close control", 0, 0, 0, 0);
		goto out;
	}

	for (dir = OUT; dir <= IN; dir++) {
		if (!(flag & (dir == OUT ? FWRITE : FREAD)))
			continue;
		sce = &sc->sc_endpoints[endpt][dir];
		if (sce->pipeh == NULL)
			continue;
		DPRINTFN(5, "endpt=%jd dir=%jd sce=%jx",
			     endpt, dir, (uintptr_t)sce, 0);

		usbd_abort_pipe(sce->pipeh);

		int isize = UGETW(sce->edesc->wMaxPacketSize);
		int msize = 0;

		switch (sce->edesc->bmAttributes & UE_XFERTYPE) {
		case UE_INTERRUPT:
			ndflush(&sce->q, sce->q.c_cc);
			clfree(&sce->q);
			msize = isize;
			break;
		case UE_ISOCHRONOUS:
			for (i = 0; i < UGEN_NISOREQS; ++i) {
				usbd_destroy_xfer(sce->isoreqs[i].xfer);
				sce->isoreqs[i].xfer = NULL;
			}
			msize = isize * UGEN_NISOFRAMES;
			break;
		case UE_BULK:
			if (sce->state & (UGEN_BULK_RA | UGEN_BULK_WB)) {
				usbd_destroy_xfer(sce->ra_wb_xfer);
				sce->ra_wb_xfer = NULL;
				msize = sce->ra_wb_bufsize;
			}
			break;
		default:
			break;
		}
		usbd_close_pipe(sce->pipeh);
		sce->pipeh = NULL;
		if (sce->ibuf != NULL) {
			kmem_free(sce->ibuf, msize);
			sce->ibuf = NULL;
		}
	}

out:	sc->sc_is_open[endpt] = 0;
	for (dir = OUT; dir <= IN; dir++) {
		sce = &sc->sc_endpoints[endpt][dir];
		KASSERT(sce->pipeh == NULL);
		KASSERT(sce->ibuf == NULL);
		KASSERT(sce->ra_wb_xfer == NULL);
		for (i = 0; i < UGEN_NISOREQS; i++)
			KASSERT(sce->isoreqs[i].xfer == NULL);
	}
}

static int
ugenclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	int endpt = UGENENDPOINT(dev);
	struct ugen_softc *sc;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("flag=%jd, mode=%jd, unit=%jd, endpt=%jd",
	    flag, mode, UGENUNIT(dev), endpt);

	KASSERT(KERNEL_LOCKED_P()); /* ugen_do_close */

	if ((sc = ugenif_acquire(UGENUNIT(dev))) == NULL)
		return ENXIO;

	KASSERT(sc->sc_is_open[endpt]);
	ugen_do_close(sc, flag, endpt);
	KASSERT(!sc->sc_is_open[endpt]);

	ugenif_release(sc);

	return 0;
}

Static int
ugen_do_read(struct ugen_softc *sc, int endpt, struct uio *uio, int flag)
{
	struct ugen_endpoint *sce = &sc->sc_endpoints[endpt][IN];
	uint32_t n, tn;
	struct usbd_xfer *xfer;
	usbd_status err;
	int error = 0;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("ugen%d: %jd", device_unit(sc->sc_dev), endpt, 0, 0);

	if (endpt == USB_CONTROL_ENDPOINT)
		return ENODEV;

	KASSERT(sce->edesc);
	KASSERT(sce->pipeh);

	switch (sce->edesc->bmAttributes & UE_XFERTYPE) {
	case UE_INTERRUPT:
		/* Block until activity occurred. */
		mutex_enter(&sc->sc_lock);
		while (sce->q.c_cc == 0) {
			if (flag & IO_NDELAY) {
				mutex_exit(&sc->sc_lock);
				return EWOULDBLOCK;
			}
			DPRINTFN(5, "sleep on %jx", (uintptr_t)sce, 0, 0, 0);
			/* "ugenri" */
			error = cv_timedwait_sig(&sce->cv, &sc->sc_lock,
			    mstohz(sce->timeout));
			DPRINTFN(5, "woke, error=%jd",
				    error, 0, 0, 0);
			if (sc->sc_dying)
				error = EIO;
			if (error)
				break;
		}
		mutex_exit(&sc->sc_lock);

		/* Transfer as many chunks as possible. */
		while (sce->q.c_cc > 0 && uio->uio_resid > 0 && !error) {
			n = uimin(sce->q.c_cc, uio->uio_resid);
			if (n > sizeof(sc->sc_buffer))
				n = sizeof(sc->sc_buffer);

			/* Remove a small chunk from the input queue. */
			q_to_b(&sce->q, sc->sc_buffer, n);
			DPRINTFN(5, "got %jd chars", n, 0, 0, 0);

			/* Copy the data to the user process. */
			error = uiomove(sc->sc_buffer, n, uio);
			if (error)
				break;
		}
		break;
	case UE_BULK:
		if (sce->state & UGEN_BULK_RA) {
			DPRINTFN(5, "BULK_RA req: %zd used: %d",
				     uio->uio_resid, sce->ra_wb_used, 0, 0);
			xfer = sce->ra_wb_xfer;

			mutex_enter(&sc->sc_lock);
			if (sce->ra_wb_used == 0 && flag & IO_NDELAY) {
				mutex_exit(&sc->sc_lock);
				return EWOULDBLOCK;
			}
			while (uio->uio_resid > 0 && !error) {
				while (sce->ra_wb_used == 0) {
					DPRINTFN(5, "sleep on %jx",
						    (uintptr_t)sce, 0, 0, 0);
					/* "ugenrb" */
					error = cv_timedwait_sig(&sce->cv,
					    &sc->sc_lock, mstohz(sce->timeout));
					DPRINTFN(5, "woke, error=%jd",
						    error, 0, 0, 0);
					if (sc->sc_dying)
						error = EIO;
					if (error)
						break;
				}

				/* Copy data to the process. */
				while (uio->uio_resid > 0
				       && sce->ra_wb_used > 0) {
					n = uimin(uio->uio_resid,
						sce->ra_wb_used);
					n = uimin(n, sce->limit - sce->cur);
					error = uiomove(sce->cur, n, uio);
					if (error)
						break;
					sce->cur += n;
					sce->ra_wb_used -= n;
					if (sce->cur == sce->limit)
						sce->cur = sce->ibuf;
				}

				/*
				 * If the transfers stopped because the
				 * buffer was full, restart them.
				 */
				if (sce->state & UGEN_RA_WB_STOP &&
				    sce->ra_wb_used < sce->limit - sce->ibuf) {
					n = (sce->limit - sce->ibuf)
					    - sce->ra_wb_used;
					usbd_setup_xfer(xfer, sce, NULL,
					    uimin(n, sce->ra_wb_xferlen),
					    0, USBD_NO_TIMEOUT,
					    ugen_bulkra_intr);
					sce->state &= ~UGEN_RA_WB_STOP;
					err = usbd_transfer(xfer);
					if (err != USBD_IN_PROGRESS)
						/*
						 * The transfer has not been
						 * queued.  Setting STOP
						 * will make us try
						 * again at the next read.
						 */
						sce->state |= UGEN_RA_WB_STOP;
				}
			}
			mutex_exit(&sc->sc_lock);
			break;
		}
		error = usbd_create_xfer(sce->pipeh, UGEN_BBSIZE,
		    0, 0, &xfer);
		if (error)
			return error;
		while ((n = uimin(UGEN_BBSIZE, uio->uio_resid)) != 0) {
			DPRINTFN(1, "start transfer %jd bytes", n, 0, 0, 0);
			tn = n;
			err = usbd_bulk_transfer(xfer, sce->pipeh,
			    sce->state & UGEN_SHORT_OK ? USBD_SHORT_XFER_OK : 0,
			    sce->timeout, sc->sc_buffer, &tn);
			if (err) {
				if (err == USBD_INTERRUPTED)
					error = EINTR;
				else if (err == USBD_TIMEOUT)
					error = ETIMEDOUT;
				else
					error = EIO;
				break;
			}
			DPRINTFN(1, "got %jd bytes", tn, 0, 0, 0);
			error = uiomove(sc->sc_buffer, tn, uio);
			if (error || tn < n)
				break;
		}
		usbd_destroy_xfer(xfer);
		break;
	case UE_ISOCHRONOUS:
		mutex_enter(&sc->sc_lock);
		while (sce->cur == sce->fill) {
			if (flag & IO_NDELAY) {
				mutex_exit(&sc->sc_lock);
				return EWOULDBLOCK;
			}
			/* "ugenri" */
			DPRINTFN(5, "sleep on %jx", (uintptr_t)sce, 0, 0, 0);
			error = cv_timedwait_sig(&sce->cv, &sc->sc_lock,
			    mstohz(sce->timeout));
			DPRINTFN(5, "woke, error=%jd", error, 0, 0, 0);
			if (sc->sc_dying)
				error = EIO;
			if (error)
				break;
		}

		while (sce->cur != sce->fill && uio->uio_resid > 0 && !error) {
			if(sce->fill > sce->cur)
				n = uimin(sce->fill - sce->cur, uio->uio_resid);
			else
				n = uimin(sce->limit - sce->cur, uio->uio_resid);

			DPRINTFN(5, "isoc got %jd chars", n, 0, 0, 0);

			/* Copy the data to the user process. */
			error = uiomove(sce->cur, n, uio);
			if (error)
				break;
			sce->cur += n;
			if (sce->cur >= sce->limit)
				sce->cur = sce->ibuf;
		}
		mutex_exit(&sc->sc_lock);
		break;


	default:
		return ENXIO;
	}
	return error;
}

static int
ugenread(dev_t dev, struct uio *uio, int flag)
{
	int endpt = UGENENDPOINT(dev);
	struct ugen_softc *sc;
	int error;

	if ((sc = ugenif_acquire(UGENUNIT(dev))) == NULL)
		return ENXIO;
	error = ugen_do_read(sc, endpt, uio, flag);
	ugenif_release(sc);

	return error;
}

Static int
ugen_do_write(struct ugen_softc *sc, int endpt, struct uio *uio,
	int flag)
{
	struct ugen_endpoint *sce = &sc->sc_endpoints[endpt][OUT];
	uint32_t n;
	int error = 0;
	uint32_t tn;
	char *dbuf;
	struct usbd_xfer *xfer;
	usbd_status err;

	UGENHIST_FUNC();
	UGENHIST_CALLARGSN(5, "ugen%jd: %jd",
	    device_unit(sc->sc_dev), endpt, 0, 0);

	if (endpt == USB_CONTROL_ENDPOINT)
		return ENODEV;

	KASSERT(sce->edesc);
	KASSERT(sce->pipeh);

	switch (sce->edesc->bmAttributes & UE_XFERTYPE) {
	case UE_BULK:
		if (sce->state & UGEN_BULK_WB) {
			DPRINTFN(5, "BULK_WB req: %jd used: %jd",
				     uio->uio_resid, sce->ra_wb_used, 0, 0);
			xfer = sce->ra_wb_xfer;

			mutex_enter(&sc->sc_lock);
			if (sce->ra_wb_used == sce->limit - sce->ibuf &&
			    flag & IO_NDELAY) {
				mutex_exit(&sc->sc_lock);
				return EWOULDBLOCK;
			}
			while (uio->uio_resid > 0 && !error) {
				while (sce->ra_wb_used ==
				       sce->limit - sce->ibuf) {
					DPRINTFN(5, "sleep on %#jx",
						     (uintptr_t)sce, 0, 0, 0);
					/* "ugenwb" */
					error = cv_timedwait_sig(&sce->cv,
					    &sc->sc_lock, mstohz(sce->timeout));
					DPRINTFN(5, "woke, error=%d",
						    error, 0, 0, 0);
					if (sc->sc_dying)
						error = EIO;
					if (error)
						break;
				}

				/* Copy data from the process. */
				while (uio->uio_resid > 0 &&
				    sce->ra_wb_used < sce->limit - sce->ibuf) {
					n = uimin(uio->uio_resid,
						(sce->limit - sce->ibuf)
						 - sce->ra_wb_used);
					n = uimin(n, sce->limit - sce->fill);
					error = uiomove(sce->fill, n, uio);
					if (error)
						break;
					sce->fill += n;
					sce->ra_wb_used += n;
					if (sce->fill == sce->limit)
						sce->fill = sce->ibuf;
				}

				/*
				 * If the transfers stopped because the
				 * buffer was empty, restart them.
				 */
				if (sce->state & UGEN_RA_WB_STOP &&
				    sce->ra_wb_used > 0) {
					dbuf = (char *)usbd_get_buffer(xfer);
					n = uimin(sce->ra_wb_used,
						sce->ra_wb_xferlen);
					tn = uimin(n, sce->limit - sce->cur);
					memcpy(dbuf, sce->cur, tn);
					dbuf += tn;
					if (n - tn > 0)
						memcpy(dbuf, sce->ibuf,
						       n - tn);
					usbd_setup_xfer(xfer, sce, NULL, n,
					    0, USBD_NO_TIMEOUT,
					    ugen_bulkwb_intr);
					sce->state &= ~UGEN_RA_WB_STOP;
					err = usbd_transfer(xfer);
					if (err != USBD_IN_PROGRESS)
						/*
						 * The transfer has not been
						 * queued.  Setting STOP
						 * will make us try again
						 * at the next read.
						 */
						sce->state |= UGEN_RA_WB_STOP;
				}
			}
			mutex_exit(&sc->sc_lock);
			break;
		}
		error = usbd_create_xfer(sce->pipeh, UGEN_BBSIZE,
		    0, 0, &xfer);
		if (error)
			return error;
		while ((n = uimin(UGEN_BBSIZE, uio->uio_resid)) != 0) {
			error = uiomove(sc->sc_buffer, n, uio);
			if (error)
				break;
			DPRINTFN(1, "transfer %jd bytes", n, 0, 0, 0);
			err = usbd_bulk_transfer(xfer, sce->pipeh, 0, sce->timeout,
			    sc->sc_buffer, &n);
			if (err) {
				if (err == USBD_INTERRUPTED)
					error = EINTR;
				else if (err == USBD_TIMEOUT)
					error = ETIMEDOUT;
				else
					error = EIO;
				break;
			}
		}
		usbd_destroy_xfer(xfer);
		break;
	case UE_INTERRUPT:
		error = usbd_create_xfer(sce->pipeh,
		    UGETW(sce->edesc->wMaxPacketSize), 0, 0, &xfer);
		if (error)
			return error;
		while ((n = uimin(UGETW(sce->edesc->wMaxPacketSize),
		    uio->uio_resid)) != 0) {
			error = uiomove(sc->sc_buffer, n, uio);
			if (error)
				break;
			DPRINTFN(1, "transfer %jd bytes", n, 0, 0, 0);
			err = usbd_intr_transfer(xfer, sce->pipeh, 0,
			    sce->timeout, sc->sc_buffer, &n);
			if (err) {
				if (err == USBD_INTERRUPTED)
					error = EINTR;
				else if (err == USBD_TIMEOUT)
					error = ETIMEDOUT;
				else
					error = EIO;
				break;
			}
		}
		usbd_destroy_xfer(xfer);
		break;
	default:
		return ENXIO;
	}
	return error;
}

static int
ugenwrite(dev_t dev, struct uio *uio, int flag)
{
	int endpt = UGENENDPOINT(dev);
	struct ugen_softc *sc;
	int error;

	if ((sc = ugenif_acquire(UGENUNIT(dev))) == NULL)
		return ENXIO;
	error = ugen_do_write(sc, endpt, uio, flag);
	ugenif_release(sc);

	return error;
}

static int
ugen_activate(device_t self, enum devact act)
{
	struct ugen_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static int
ugen_detach(device_t self, int flags)
{
	struct ugen_softc *sc = device_private(self);
	struct ugen_endpoint *sce;
	int i, dir;
	int maj, mn;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("sc=%ju flags=%ju", (uintptr_t)sc, flags, 0, 0);

	KASSERT(KERNEL_LOCKED_P()); /* sc_is_open */

	/*
	 * Fail if we're not forced to detach and userland has any
	 * endpoints open.
	 */
	if ((flags & DETACH_FORCE) == 0) {
		for (i = 0; i < USB_MAX_ENDPOINTS; i++) {
			if (sc->sc_is_open[i])
				return EBUSY;
		}
	}

	/* Prevent new users.  Prevent suspend/resume.  */
	sc->sc_dying = 1;
	pmf_device_deregister(self);

	/*
	 * If we never finished attaching, skip nixing endpoints and
	 * users because there aren't any.
	 */
	if (!sc->sc_attached)
		goto out;

	/* Abort all pipes.  */
	for (i = 0; i < USB_MAX_ENDPOINTS; i++) {
		for (dir = OUT; dir <= IN; dir++) {
			sce = &sc->sc_endpoints[i][dir];
			if (sce->pipeh)
				usbd_abort_pipe(sce->pipeh);
		}
	}

	/*
	 * Wait for users to drain.  Before this point there can be no
	 * more I/O operations started because we set sc_dying; after
	 * this, there can be no more I/O operations in progress, so it
	 * will be safe to free things.
	 */
	mutex_enter(&sc->sc_lock);
	if (--sc->sc_refcnt >= 0) {
		/* Wake everyone */
		for (i = 0; i < USB_MAX_ENDPOINTS; i++) {
			for (dir = OUT; dir <= IN; dir++)
				cv_broadcast(&sc->sc_endpoints[i][dir].cv);
		}
		/* Wait for processes to go away. */
		do {
			cv_wait(&sc->sc_detach_cv, &sc->sc_lock);
		} while (sc->sc_refcnt >= 0);
	}
	mutex_exit(&sc->sc_lock);

	/* locate the major number */
	maj = cdevsw_lookup_major(&ugen_cdevsw);

	/*
	 * Nuke the vnodes for any open instances (calls ugenclose, but
	 * with no effect because we already set sc_dying).
	 */
	mn = sc->sc_unit * USB_MAX_ENDPOINTS;
	vdevgone(maj, mn, mn + USB_MAX_ENDPOINTS - 1, VCHR);

	/* Actually close any lingering pipes.  */
	for (i = 0; i < USB_MAX_ENDPOINTS; i++)
		ugen_do_close(sc, FREAD|FWRITE, i);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, sc->sc_dev);
	ugenif_put_unit(sc);

out:	for (i = 0; i < USB_MAX_ENDPOINTS; i++) {
		for (dir = OUT; dir <= IN; dir++) {
			sce = &sc->sc_endpoints[i][dir];
			seldestroy(&sce->rsel);
			cv_destroy(&sce->cv);
		}
	}

	cv_destroy(&sc->sc_detach_cv);
	mutex_destroy(&sc->sc_lock);

	return 0;
}

Static void
ugenintr(struct usbd_xfer *xfer, void *addr, usbd_status status)
{
	struct ugen_endpoint *sce = addr;
	struct ugen_softc *sc = sce->sc;
	uint32_t count;
	u_char *ibuf;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("xfer %jx status %d", (uintptr_t)xfer, status, 0, 0);

	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF("status=%jd", status, 0, 0, 0);
		if (status == USBD_STALLED)
		    usbd_clear_endpoint_stall_async(sce->pipeh);
		return;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &count, NULL);
	ibuf = sce->ibuf;

	DPRINTFN(5, "xfer=%#jx status=%d count=%d",
		     (uintptr_t)xfer, status, count, 0);
	DPRINTFN(5, "          data = %02x %02x %02x",
		     ibuf[0], ibuf[1], ibuf[2], 0);

	mutex_enter(&sc->sc_lock);
	(void)b_to_q(ibuf, count, &sce->q);
	cv_signal(&sce->cv);
	mutex_exit(&sc->sc_lock);
	selnotify(&sce->rsel, 0, 0);
}

Static void
ugen_isoc_rintr(struct usbd_xfer *xfer, void *addr,
		usbd_status status)
{
	struct isoreq *req = addr;
	struct ugen_endpoint *sce = req->sce;
	struct ugen_softc *sc = sce->sc;
	uint32_t count, n;
	int i, isize;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("xfer=%jx status=%jd", (uintptr_t)xfer, status, 0, 0);

	/* Return if we are aborting. */
	if (status == USBD_CANCELLED)
		return;

	usbd_get_xfer_status(xfer, NULL, NULL, &count, NULL);
	DPRINTFN(5, "xfer %ld, count=%d",
	    (long)(req - sce->isoreqs), count, 0, 0);

	mutex_enter(&sc->sc_lock);

	/* throw away oldest input if the buffer is full */
	if (sce->fill < sce->cur && sce->cur <= sce->fill + count) {
		sce->cur += count;
		if (sce->cur >= sce->limit)
			sce->cur = sce->ibuf + (sce->limit - sce->cur);
		DPRINTFN(5, "throwing away %jd bytes",
			     count, 0, 0, 0);
	}

	isize = UGETW(sce->edesc->wMaxPacketSize);
	for (i = 0; i < UGEN_NISORFRMS; i++) {
		uint32_t actlen = req->sizes[i];
		char const *tbuf = (char const *)req->dmabuf + isize * i;

		/* copy data to buffer */
		while (actlen > 0) {
			n = uimin(actlen, sce->limit - sce->fill);
			memcpy(sce->fill, tbuf, n);

			tbuf += n;
			actlen -= n;
			sce->fill += n;
			if (sce->fill == sce->limit)
				sce->fill = sce->ibuf;
		}

		/* setup size for next transfer */
		req->sizes[i] = isize;
	}

	usbd_setup_isoc_xfer(xfer, req, req->sizes, UGEN_NISORFRMS, 0,
	    ugen_isoc_rintr);
	(void)usbd_transfer(xfer);

	cv_signal(&sce->cv);
	mutex_exit(&sc->sc_lock);
	selnotify(&sce->rsel, 0, 0);
}

Static void
ugen_bulkra_intr(struct usbd_xfer *xfer, void *addr,
		 usbd_status status)
{
	struct ugen_endpoint *sce = addr;
	struct ugen_softc *sc = sce->sc;
	uint32_t count, n;
	char const *tbuf;
	usbd_status err;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("xfer=%jx status=%jd", (uintptr_t)xfer, status, 0, 0);

	/* Return if we are aborting. */
	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF("status=%jd", status, 0, 0, 0);
		sce->state |= UGEN_RA_WB_STOP;
		if (status == USBD_STALLED)
		    usbd_clear_endpoint_stall_async(sce->pipeh);
		return;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &count, NULL);

	mutex_enter(&sc->sc_lock);

	/* Keep track of how much is in the buffer. */
	sce->ra_wb_used += count;

	/* Copy data to buffer. */
	tbuf = (char const *)usbd_get_buffer(sce->ra_wb_xfer);
	n = uimin(count, sce->limit - sce->fill);
	memcpy(sce->fill, tbuf, n);
	tbuf += n;
	count -= n;
	sce->fill += n;
	if (sce->fill == sce->limit)
		sce->fill = sce->ibuf;
	if (count > 0) {
		memcpy(sce->fill, tbuf, count);
		sce->fill += count;
	}

	/* Set up the next request if necessary. */
	n = (sce->limit - sce->ibuf) - sce->ra_wb_used;
	if (n > 0) {
		usbd_setup_xfer(xfer, sce, NULL, uimin(n, sce->ra_wb_xferlen), 0,
		    USBD_NO_TIMEOUT, ugen_bulkra_intr);
		err = usbd_transfer(xfer);
		if (err != USBD_IN_PROGRESS) {
			printf("error=%d", err);
			/*
			 * The transfer has not been queued.  Setting STOP
			 * will make us try again at the next read.
			 */
			sce->state |= UGEN_RA_WB_STOP;
		}
	}
	else
		sce->state |= UGEN_RA_WB_STOP;

	cv_signal(&sce->cv);
	mutex_exit(&sc->sc_lock);
	selnotify(&sce->rsel, 0, 0);
}

Static void
ugen_bulkwb_intr(struct usbd_xfer *xfer, void *addr,
		 usbd_status status)
{
	struct ugen_endpoint *sce = addr;
	struct ugen_softc *sc = sce->sc;
	uint32_t count, n;
	char *tbuf;
	usbd_status err;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("xfer=%jx status=%jd", (uintptr_t)xfer, status, 0, 0);

	/* Return if we are aborting. */
	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF("status=%jd", status, 0, 0, 0);
		sce->state |= UGEN_RA_WB_STOP;
		if (status == USBD_STALLED)
		    usbd_clear_endpoint_stall_async(sce->pipeh);
		return;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &count, NULL);

	mutex_enter(&sc->sc_lock);

	/* Keep track of how much is in the buffer. */
	sce->ra_wb_used -= count;

	/* Update buffer pointers. */
	sce->cur += count;
	if (sce->cur >= sce->limit)
		sce->cur = sce->ibuf + (sce->cur - sce->limit);

	/* Set up next request if necessary. */
	if (sce->ra_wb_used > 0) {
		/* copy data from buffer */
		tbuf = (char *)usbd_get_buffer(sce->ra_wb_xfer);
		count = uimin(sce->ra_wb_used, sce->ra_wb_xferlen);
		n = uimin(count, sce->limit - sce->cur);
		memcpy(tbuf, sce->cur, n);
		tbuf += n;
		if (count - n > 0)
			memcpy(tbuf, sce->ibuf, count - n);

		usbd_setup_xfer(xfer, sce, NULL, count, 0, USBD_NO_TIMEOUT,
		    ugen_bulkwb_intr);
		err = usbd_transfer(xfer);
		if (err != USBD_IN_PROGRESS) {
			printf("error=%d", err);
			/*
			 * The transfer has not been queued.  Setting STOP
			 * will make us try again at the next write.
			 */
			sce->state |= UGEN_RA_WB_STOP;
		}
	}
	else
		sce->state |= UGEN_RA_WB_STOP;

	cv_signal(&sce->cv);
	mutex_exit(&sc->sc_lock);
	selnotify(&sce->rsel, 0, 0);
}

Static usbd_status
ugen_set_interface(struct ugen_softc *sc, int ifaceidx, int altno)
{
	struct usbd_interface *iface;
	usb_endpoint_descriptor_t *ed;
	usbd_status err;
	struct ugen_endpoint *sce;
	uint8_t niface, nendpt, endptno, endpt;
	int dir;

	UGENHIST_FUNC();
	UGENHIST_CALLARGSN(15, "ifaceidx=%jd altno=%jd", ifaceidx, altno, 0, 0);

	err = usbd_interface_count(sc->sc_udev, &niface);
	if (err)
		return err;
	if (ifaceidx < 0 || ifaceidx >= niface)
		return USBD_INVAL;

	err = usbd_device2interface_handle(sc->sc_udev, ifaceidx, &iface);
	if (err)
		return err;
	err = usbd_endpoint_count(iface, &nendpt);
	if (err)
		return err;

	/* change setting */
	err = usbd_set_interface(iface, altno);
	if (err)
		return err;

	err = usbd_endpoint_count(iface, &nendpt);
	if (err)
		return err;

	ugen_clear_endpoints(sc);

	for (endptno = 0; endptno < nendpt; endptno++) {
		ed = usbd_interface2endpoint_descriptor(iface, endptno);
		KASSERT(ed != NULL);
		endpt = ed->bEndpointAddress;
		dir = UE_GET_DIR(endpt) == UE_DIR_IN ? IN : OUT;
		sce = &sc->sc_endpoints[UE_GET_ADDR(endpt)][dir];
		sce->sc = sc;
		sce->edesc = ed;
		sce->iface = iface;
	}
	return 0;
}

/* Retrieve a complete descriptor for a certain device and index. */
Static usb_config_descriptor_t *
ugen_get_cdesc(struct ugen_softc *sc, int index, int *lenp)
{
	usb_config_descriptor_t *cdesc = NULL, *tdesc, cdescr;
	int len = 0;
	usbd_status err;

	UGENHIST_FUNC(); UGENHIST_CALLARGS("index=%jd", index, 0, 0, 0);

	switch (index) {
	case USB_CURRENT_CONFIG_INDEX:
		tdesc = usbd_get_config_descriptor(sc->sc_udev);
		if (tdesc == NULL)
			break;
		len = UGETW(tdesc->wTotalLength);
		cdesc = kmem_alloc(len, KM_SLEEP);
		memcpy(cdesc, tdesc, len);
		break;
	default:
		err = usbd_get_config_desc(sc->sc_udev, index, &cdescr);
		if (err)
			break;
		len = UGETW(cdescr.wTotalLength);
		cdesc = kmem_alloc(len, KM_SLEEP);
		err = usbd_get_config_desc_full(sc->sc_udev, index, cdesc, len);
		if (err) {
			kmem_free(cdesc, len);
			cdesc = NULL;
		}
		break;
	}
	DPRINTFN(5, "req len=%jd cdesc=%jx", len, (uintptr_t)cdesc, 0, 0);
	if (cdesc && lenp)
		*lenp = len;
	return cdesc;
}

Static int
ugen_get_alt_index(struct ugen_softc *sc, int ifaceidx)
{
	struct usbd_interface *iface;
	usbd_status err;

	err = usbd_device2interface_handle(sc->sc_udev, ifaceidx, &iface);
	if (err)
		return -1;
	return usbd_get_interface_altindex(iface);
}

Static int
ugen_do_ioctl(struct ugen_softc *sc, int endpt, u_long cmd,
	      void *addr, int flag, struct lwp *l)
{
	struct ugen_endpoint *sce;
	usbd_status err;
	struct usbd_interface *iface;
	struct usb_config_desc *cd;
	usb_config_descriptor_t *cdesc;
	struct usb_interface_desc *id;
	usb_interface_descriptor_t *idesc;
	struct usb_endpoint_desc *ed;
	usb_endpoint_descriptor_t *edesc;
	struct usb_alt_interface *ai;
	struct usb_string_desc *si;
	uint8_t conf, alt;
	int cdesclen;
	int error;
	int dir;

	UGENHIST_FUNC();
	UGENHIST_CALLARGS("ugen%d: endpt=%ju cmd=%08jx flag=%jx",
	    device_unit(sc->sc_dev), endpt, cmd, flag);

	KASSERT(KERNEL_LOCKED_P()); /* ugen_set_config */

	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		return 0;
	case USB_SET_SHORT_XFER:
		if (endpt == USB_CONTROL_ENDPOINT)
			return EINVAL;
		/* This flag only affects read */
		sce = &sc->sc_endpoints[endpt][IN];
		if (sce == NULL || sce->pipeh == NULL)
			return EINVAL;
		if (*(int *)addr)
			sce->state |= UGEN_SHORT_OK;
		else
			sce->state &= ~UGEN_SHORT_OK;
		DPRINTFN(5, "pipe=%jx short xfer=%ju",
		    (uintptr_t)sce->pipeh, sce->state & UGEN_SHORT_OK, 0, 0);
		return 0;
	case USB_SET_TIMEOUT:
		for (dir = OUT; dir <= IN; dir++) {
			sce = &sc->sc_endpoints[endpt][dir];
			if (sce == NULL)
				return EINVAL;

			sce->timeout = *(int *)addr;
			DPRINTFN(5, "pipe=%jx timeout[dir=%ju] timeout=%ju",
			    (uintptr_t)sce->pipeh, dir, sce->timeout, 0);
		}
		return 0;
	case USB_SET_BULK_RA:
		if (endpt == USB_CONTROL_ENDPOINT)
			return EINVAL;
		sce = &sc->sc_endpoints[endpt][IN];
		if (sce == NULL || sce->pipeh == NULL)
			return EINVAL;
		edesc = sce->edesc;
		if ((edesc->bmAttributes & UE_XFERTYPE) != UE_BULK)
			return EINVAL;

		if (*(int *)addr) {
			/* Only turn RA on if it's currently off. */
			if (sce->state & UGEN_BULK_RA)
				return 0;
			KASSERT(sce->ra_wb_xfer == NULL);
			KASSERT(sce->ibuf == NULL);

			if (sce->ra_wb_bufsize == 0 || sce->ra_wb_reqsize == 0)
				/* shouldn't happen */
				return EINVAL;
			error = usbd_create_xfer(sce->pipeh,
			    sce->ra_wb_reqsize, 0, 0, &sce->ra_wb_xfer);
			if (error)
				return error;
			sce->ra_wb_xferlen = sce->ra_wb_reqsize;
			sce->ibuf = kmem_alloc(sce->ra_wb_bufsize, KM_SLEEP);
			sce->fill = sce->cur = sce->ibuf;
			sce->limit = sce->ibuf + sce->ra_wb_bufsize;
			sce->ra_wb_used = 0;
			sce->state |= UGEN_BULK_RA;
			sce->state &= ~UGEN_RA_WB_STOP;
			/* Now start reading. */
			usbd_setup_xfer(sce->ra_wb_xfer, sce, NULL,
			    uimin(sce->ra_wb_xferlen, sce->ra_wb_bufsize),
			     0, USBD_NO_TIMEOUT, ugen_bulkra_intr);
			err = usbd_transfer(sce->ra_wb_xfer);
			if (err != USBD_IN_PROGRESS) {
				sce->state &= ~UGEN_BULK_RA;
				kmem_free(sce->ibuf, sce->ra_wb_bufsize);
				sce->ibuf = NULL;
				usbd_destroy_xfer(sce->ra_wb_xfer);
				sce->ra_wb_xfer = NULL;
				return EIO;
			}
		} else {
			/* Only turn RA off if it's currently on. */
			if (!(sce->state & UGEN_BULK_RA))
				return 0;

			sce->state &= ~UGEN_BULK_RA;
			usbd_abort_pipe(sce->pipeh);
			usbd_destroy_xfer(sce->ra_wb_xfer);
			sce->ra_wb_xfer = NULL;
			/*
			 * XXX Discard whatever's in the buffer, but we
			 * should keep it around and drain the buffer
			 * instead.
			 */
			kmem_free(sce->ibuf, sce->ra_wb_bufsize);
			sce->ibuf = NULL;
		}
		return 0;
	case USB_SET_BULK_WB:
		if (endpt == USB_CONTROL_ENDPOINT)
			return EINVAL;
		sce = &sc->sc_endpoints[endpt][OUT];
		if (sce == NULL || sce->pipeh == NULL)
			return EINVAL;
		edesc = sce->edesc;
		if ((edesc->bmAttributes & UE_XFERTYPE) != UE_BULK)
			return EINVAL;

		if (*(int *)addr) {
			/* Only turn WB on if it's currently off. */
			if (sce->state & UGEN_BULK_WB)
				return 0;
			KASSERT(sce->ra_wb_xfer == NULL);
			KASSERT(sce->ibuf == NULL);

			if (sce->ra_wb_bufsize == 0 || sce->ra_wb_reqsize == 0)
				/* shouldn't happen */
				return EINVAL;
			error = usbd_create_xfer(sce->pipeh, sce->ra_wb_reqsize,
			    0, 0, &sce->ra_wb_xfer);
			/* XXX check error???  */
			sce->ra_wb_xferlen = sce->ra_wb_reqsize;
			sce->ibuf = kmem_alloc(sce->ra_wb_bufsize, KM_SLEEP);
			sce->fill = sce->cur = sce->ibuf;
			sce->limit = sce->ibuf + sce->ra_wb_bufsize;
			sce->ra_wb_used = 0;
			sce->state |= UGEN_BULK_WB | UGEN_RA_WB_STOP;
		} else {
			/* Only turn WB off if it's currently on. */
			if (!(sce->state & UGEN_BULK_WB))
				return 0;

			sce->state &= ~UGEN_BULK_WB;
			/*
			 * XXX Discard whatever's in the buffer, but we
			 * should keep it around and keep writing to
			 * drain the buffer instead.
			 */
			usbd_abort_pipe(sce->pipeh);
			usbd_destroy_xfer(sce->ra_wb_xfer);
			sce->ra_wb_xfer = NULL;
			kmem_free(sce->ibuf, sce->ra_wb_bufsize);
			sce->ibuf = NULL;
		}
		return 0;
	case USB_SET_BULK_RA_OPT:
	case USB_SET_BULK_WB_OPT:
	{
		struct usb_bulk_ra_wb_opt *opt;

		if (endpt == USB_CONTROL_ENDPOINT)
			return EINVAL;
		opt = (struct usb_bulk_ra_wb_opt *)addr;
		if (cmd == USB_SET_BULK_RA_OPT)
			sce = &sc->sc_endpoints[endpt][IN];
		else
			sce = &sc->sc_endpoints[endpt][OUT];
		if (sce == NULL || sce->pipeh == NULL)
			return EINVAL;
		if (opt->ra_wb_buffer_size < 1 ||
		    opt->ra_wb_buffer_size > UGEN_BULK_RA_WB_BUFMAX ||
		    opt->ra_wb_request_size < 1 ||
		    opt->ra_wb_request_size > opt->ra_wb_buffer_size)
			return EINVAL;
		/*
		 * XXX These changes do not take effect until the
		 * next time RA/WB mode is enabled but they ought to
		 * take effect immediately.
		 */
		sce->ra_wb_bufsize = opt->ra_wb_buffer_size;
		sce->ra_wb_reqsize = opt->ra_wb_request_size;
		return 0;
	}
	default:
		break;
	}

	if (endpt != USB_CONTROL_ENDPOINT)
		return EINVAL;

	switch (cmd) {
#ifdef UGEN_DEBUG
	case USB_SETDEBUG:
		ugendebug = *(int *)addr;
		break;
#endif
	case USB_GET_CONFIG:
		err = usbd_get_config(sc->sc_udev, &conf);
		if (err)
			return EIO;
		*(int *)addr = conf;
		break;
	case USB_SET_CONFIG:
		if (!(flag & FWRITE))
			return EPERM;
		err = ugen_set_config(sc, *(int *)addr, 1);
		switch (err) {
		case USBD_NORMAL_COMPLETION:
			break;
		case USBD_IN_USE:
			return EBUSY;
		default:
			return EIO;
		}
		break;
	case USB_GET_ALTINTERFACE:
		ai = (struct usb_alt_interface *)addr;
		err = usbd_device2interface_handle(sc->sc_udev,
			  ai->uai_interface_index, &iface);
		if (err)
			return EINVAL;
		idesc = usbd_get_interface_descriptor(iface);
		if (idesc == NULL)
			return EIO;
		ai->uai_alt_no = idesc->bAlternateSetting;
		break;
	case USB_SET_ALTINTERFACE:
		if (!(flag & FWRITE))
			return EPERM;
		ai = (struct usb_alt_interface *)addr;
		err = usbd_device2interface_handle(sc->sc_udev,
			  ai->uai_interface_index, &iface);
		if (err)
			return EINVAL;
		err = ugen_set_interface(sc, ai->uai_interface_index,
		    ai->uai_alt_no);
		if (err)
			return EINVAL;
		break;
	case USB_GET_NO_ALT:
		ai = (struct usb_alt_interface *)addr;
		cdesc = ugen_get_cdesc(sc, ai->uai_config_index, &cdesclen);
		if (cdesc == NULL)
			return EINVAL;
		idesc = usbd_find_idesc(cdesc, ai->uai_interface_index, 0);
		if (idesc == NULL) {
			kmem_free(cdesc, cdesclen);
			return EINVAL;
		}
		ai->uai_alt_no = usbd_get_no_alts(cdesc,
		    idesc->bInterfaceNumber);
		kmem_free(cdesc, cdesclen);
		break;
	case USB_GET_DEVICE_DESC:
		*(usb_device_descriptor_t *)addr =
			*usbd_get_device_descriptor(sc->sc_udev);
		break;
	case USB_GET_CONFIG_DESC:
		cd = (struct usb_config_desc *)addr;
		cdesc = ugen_get_cdesc(sc, cd->ucd_config_index, &cdesclen);
		if (cdesc == NULL)
			return EINVAL;
		cd->ucd_desc = *cdesc;
		kmem_free(cdesc, cdesclen);
		break;
	case USB_GET_INTERFACE_DESC:
		id = (struct usb_interface_desc *)addr;
		cdesc = ugen_get_cdesc(sc, id->uid_config_index, &cdesclen);
		if (cdesc == NULL)
			return EINVAL;
		if (id->uid_config_index == USB_CURRENT_CONFIG_INDEX &&
		    id->uid_alt_index == USB_CURRENT_ALT_INDEX)
			alt = ugen_get_alt_index(sc, id->uid_interface_index);
		else
			alt = id->uid_alt_index;
		idesc = usbd_find_idesc(cdesc, id->uid_interface_index, alt);
		if (idesc == NULL) {
			kmem_free(cdesc, cdesclen);
			return EINVAL;
		}
		id->uid_desc = *idesc;
		kmem_free(cdesc, cdesclen);
		break;
	case USB_GET_ENDPOINT_DESC:
		ed = (struct usb_endpoint_desc *)addr;
		cdesc = ugen_get_cdesc(sc, ed->ued_config_index, &cdesclen);
		if (cdesc == NULL)
			return EINVAL;
		if (ed->ued_config_index == USB_CURRENT_CONFIG_INDEX &&
		    ed->ued_alt_index == USB_CURRENT_ALT_INDEX)
			alt = ugen_get_alt_index(sc, ed->ued_interface_index);
		else
			alt = ed->ued_alt_index;
		edesc = usbd_find_edesc(cdesc, ed->ued_interface_index,
					alt, ed->ued_endpoint_index);
		if (edesc == NULL) {
			kmem_free(cdesc, cdesclen);
			return EINVAL;
		}
		ed->ued_desc = *edesc;
		kmem_free(cdesc, cdesclen);
		break;
	case USB_GET_FULL_DESC:
	{
		int len;
		struct iovec iov;
		struct uio uio;
		struct usb_full_desc *fd = (struct usb_full_desc *)addr;

		cdesc = ugen_get_cdesc(sc, fd->ufd_config_index, &cdesclen);
		if (cdesc == NULL)
			return EINVAL;
		len = cdesclen;
		if (len > fd->ufd_size)
			len = fd->ufd_size;
		iov.iov_base = (void *)fd->ufd_data;
		iov.iov_len = len;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_resid = len;
		uio.uio_offset = 0;
		uio.uio_rw = UIO_READ;
		uio.uio_vmspace = l->l_proc->p_vmspace;
		error = uiomove((void *)cdesc, len, &uio);
		kmem_free(cdesc, cdesclen);
		return error;
	}
	case USB_GET_STRING_DESC: {
		int len;
		si = (struct usb_string_desc *)addr;
		err = usbd_get_string_desc(sc->sc_udev, si->usd_string_index,
			  si->usd_language_id, &si->usd_desc, &len);
		if (err)
			return EINVAL;
		break;
	}
	case USB_DO_REQUEST:
	{
		struct usb_ctl_request *ur = (void *)addr;
		int len = UGETW(ur->ucr_request.wLength);
		struct iovec iov;
		struct uio uio;
		void *ptr = 0;
		usbd_status xerr;

		error = 0;

		if (!(flag & FWRITE))
			return EPERM;
		/* Avoid requests that would damage the bus integrity. */
		if ((ur->ucr_request.bmRequestType == UT_WRITE_DEVICE &&
		     ur->ucr_request.bRequest == UR_SET_ADDRESS) ||
		    (ur->ucr_request.bmRequestType == UT_WRITE_DEVICE &&
		     ur->ucr_request.bRequest == UR_SET_CONFIG) ||
		    (ur->ucr_request.bmRequestType == UT_WRITE_INTERFACE &&
		     ur->ucr_request.bRequest == UR_SET_INTERFACE))
			return EINVAL;

		if (len < 0 || len > 32767)
			return EINVAL;
		if (len != 0) {
			iov.iov_base = (void *)ur->ucr_data;
			iov.iov_len = len;
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_resid = len;
			uio.uio_offset = 0;
			uio.uio_rw =
				ur->ucr_request.bmRequestType & UT_READ ?
				UIO_READ : UIO_WRITE;
			uio.uio_vmspace = l->l_proc->p_vmspace;
			ptr = kmem_alloc(len, KM_SLEEP);
			if (uio.uio_rw == UIO_WRITE) {
				error = uiomove(ptr, len, &uio);
				if (error)
					goto ret;
			}
		}
		sce = &sc->sc_endpoints[endpt][IN];
		xerr = usbd_do_request_flags(sc->sc_udev, &ur->ucr_request,
			  ptr, ur->ucr_flags, &ur->ucr_actlen, sce->timeout);
		if (xerr) {
			error = EIO;
			goto ret;
		}
		if (len != 0) {
			if (uio.uio_rw == UIO_READ) {
				size_t alen = uimin(len, ur->ucr_actlen);
				error = uiomove(ptr, alen, &uio);
				if (error)
					goto ret;
			}
		}
	ret:
		if (ptr)
			kmem_free(ptr, len);
		return error;
	}
	case USB_GET_DEVICEINFO:
		usbd_fill_deviceinfo(sc->sc_udev,
				     (struct usb_device_info *)addr, 0);
		break;
	case USB_GET_DEVICEINFO_30:
	{
		int ret;
		MODULE_HOOK_CALL(usb_subr_fill_30_hook,
		    (sc->sc_udev, (struct usb_device_info30 *)addr, 0,
		      usbd_devinfo_vp, usbd_printBCD),
		    enosys(), ret);
		if (ret == 0)
			return 0;
		return EINVAL;
	}
	default:
		return EINVAL;
	}
	return 0;
}

static int
ugenioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	int endpt = UGENENDPOINT(dev);
	struct ugen_softc *sc;
	int error;

	if ((sc = ugenif_acquire(UGENUNIT(dev))) == 0)
		return ENXIO;
	error = ugen_do_ioctl(sc, endpt, cmd, addr, flag, l);
	ugenif_release(sc);

	return error;
}

static int
ugenpoll(dev_t dev, int events, struct lwp *l)
{
	struct ugen_softc *sc;
	struct ugen_endpoint *sce_in, *sce_out;
	int revents = 0;

	if ((sc = ugenif_acquire(UGENUNIT(dev))) == NULL)
		return POLLHUP;

	if (UGENENDPOINT(dev) == USB_CONTROL_ENDPOINT) {
		revents |= POLLERR;
		goto out;
	}

	sce_in = &sc->sc_endpoints[UGENENDPOINT(dev)][IN];
	sce_out = &sc->sc_endpoints[UGENENDPOINT(dev)][OUT];
	KASSERT(sce_in->edesc || sce_out->edesc);
	KASSERT(sce_in->pipeh || sce_out->pipeh);

	mutex_enter(&sc->sc_lock);
	if (sce_in && sce_in->pipeh && (events & (POLLIN | POLLRDNORM)))
		switch (sce_in->edesc->bmAttributes & UE_XFERTYPE) {
		case UE_INTERRUPT:
			if (sce_in->q.c_cc > 0)
				revents |= events & (POLLIN | POLLRDNORM);
			else
				selrecord(l, &sce_in->rsel);
			break;
		case UE_ISOCHRONOUS:
			if (sce_in->cur != sce_in->fill)
				revents |= events & (POLLIN | POLLRDNORM);
			else
				selrecord(l, &sce_in->rsel);
			break;
		case UE_BULK:
			if (sce_in->state & UGEN_BULK_RA) {
				if (sce_in->ra_wb_used > 0)
					revents |= events &
					    (POLLIN | POLLRDNORM);
				else
					selrecord(l, &sce_in->rsel);
				break;
			}
			/*
			 * We have no easy way of determining if a read will
			 * yield any data or a write will happen.
			 * Pretend they will.
			 */
			revents |= events & (POLLIN | POLLRDNORM);
			break;
		default:
			break;
		}
	if (sce_out && sce_out->pipeh && (events & (POLLOUT | POLLWRNORM)))
		switch (sce_out->edesc->bmAttributes & UE_XFERTYPE) {
		case UE_INTERRUPT:
		case UE_ISOCHRONOUS:
			/* XXX unimplemented */
			break;
		case UE_BULK:
			if (sce_out->state & UGEN_BULK_WB) {
				if (sce_out->ra_wb_used <
				    sce_out->limit - sce_out->ibuf)
					revents |= events &
					    (POLLOUT | POLLWRNORM);
				else
					selrecord(l, &sce_out->rsel);
				break;
			}
			/*
			 * We have no easy way of determining if a read will
			 * yield any data or a write will happen.
			 * Pretend they will.
			 */
			 revents |= events & (POLLOUT | POLLWRNORM);
			 break;
		default:
			break;
		}

	mutex_exit(&sc->sc_lock);

out:	ugenif_release(sc);
	return revents;
}

static void
filt_ugenrdetach(struct knote *kn)
{
	struct ugen_endpoint *sce = kn->kn_hook;
	struct ugen_softc *sc = sce->sc;

	mutex_enter(&sc->sc_lock);
	selremove_knote(&sce->rsel, kn);
	mutex_exit(&sc->sc_lock);
}

static int
filt_ugenread_intr(struct knote *kn, long hint)
{
	struct ugen_endpoint *sce = kn->kn_hook;
	struct ugen_softc *sc = sce->sc;
	int ret;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		ret = 0;
	} else {
		kn->kn_data = sce->q.c_cc;
		ret = kn->kn_data > 0;
	}
	mutex_exit(&sc->sc_lock);

	return ret;
}

static int
filt_ugenread_isoc(struct knote *kn, long hint)
{
	struct ugen_endpoint *sce = kn->kn_hook;
	struct ugen_softc *sc = sce->sc;
	int ret;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		ret = 0;
	} else if (sce->cur == sce->fill) {
		ret = 0;
	} else if (sce->cur < sce->fill) {
		kn->kn_data = sce->fill - sce->cur;
		ret = 1;
	} else {
		kn->kn_data = (sce->limit - sce->cur) +
		    (sce->fill - sce->ibuf);
		ret = 1;
	}
	mutex_exit(&sc->sc_lock);

	return ret;
}

static int
filt_ugenread_bulk(struct knote *kn, long hint)
{
	struct ugen_endpoint *sce = kn->kn_hook;
	struct ugen_softc *sc = sce->sc;
	int ret;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		ret = 0;
	} else if (!(sce->state & UGEN_BULK_RA)) {
		/*
		 * We have no easy way of determining if a read will
		 * yield any data or a write will happen.
		 * So, emulate "seltrue".
		 */
		ret = filt_seltrue(kn, hint);
	} else if (sce->ra_wb_used == 0) {
		ret = 0;
	} else {
		kn->kn_data = sce->ra_wb_used;
		ret = 1;
	}
	mutex_exit(&sc->sc_lock);

	return ret;
}

static int
filt_ugenwrite_bulk(struct knote *kn, long hint)
{
	struct ugen_endpoint *sce = kn->kn_hook;
	struct ugen_softc *sc = sce->sc;
	int ret;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		ret = 0;
	} else if (!(sce->state & UGEN_BULK_WB)) {
		/*
		 * We have no easy way of determining if a read will
		 * yield any data or a write will happen.
		 * So, emulate "seltrue".
		 */
		ret = filt_seltrue(kn, hint);
	} else if (sce->ra_wb_used == sce->limit - sce->ibuf) {
		ret = 0;
	} else {
		kn->kn_data = (sce->limit - sce->ibuf) - sce->ra_wb_used;
		ret = 1;
	}
	mutex_exit(&sc->sc_lock);

	return ret;
}

static const struct filterops ugenread_intr_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_ugenrdetach,
	.f_event = filt_ugenread_intr,
};

static const struct filterops ugenread_isoc_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_ugenrdetach,
	.f_event = filt_ugenread_isoc,
};

static const struct filterops ugenread_bulk_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_ugenrdetach,
	.f_event = filt_ugenread_bulk,
};

static const struct filterops ugenwrite_bulk_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_ugenrdetach,
	.f_event = filt_ugenwrite_bulk,
};

static int
ugenkqfilter(dev_t dev, struct knote *kn)
{
	struct ugen_softc *sc;
	struct ugen_endpoint *sce;
	struct selinfo *sip;
	int error;

	if ((sc = ugenif_acquire(UGENUNIT(dev))) == NULL)
		return ENXIO;

	if (UGENENDPOINT(dev) == USB_CONTROL_ENDPOINT) {
		error = ENODEV;
		goto out;
	}

	switch (kn->kn_filter) {
	case EVFILT_READ:
		sce = &sc->sc_endpoints[UGENENDPOINT(dev)][IN];
		if (sce == NULL) {
			error = EINVAL;
			goto out;
		}

		sip = &sce->rsel;
		switch (sce->edesc->bmAttributes & UE_XFERTYPE) {
		case UE_INTERRUPT:
			kn->kn_fop = &ugenread_intr_filtops;
			break;
		case UE_ISOCHRONOUS:
			kn->kn_fop = &ugenread_isoc_filtops;
			break;
		case UE_BULK:
			kn->kn_fop = &ugenread_bulk_filtops;
			break;
		default:
			error = EINVAL;
			goto out;
		}
		break;

	case EVFILT_WRITE:
		sce = &sc->sc_endpoints[UGENENDPOINT(dev)][OUT];
		if (sce == NULL) {
			error = EINVAL;
			goto out;
		}

		sip = &sce->rsel;
		switch (sce->edesc->bmAttributes & UE_XFERTYPE) {
		case UE_INTERRUPT:
		case UE_ISOCHRONOUS:
			/* XXX poll doesn't support this */
			error = EINVAL;
			goto out;

		case UE_BULK:
			kn->kn_fop = &ugenwrite_bulk_filtops;
			break;
		default:
			error = EINVAL;
			goto out;
		}
		break;

	default:
		error = EINVAL;
		goto out;
	}

	kn->kn_hook = sce;

	mutex_enter(&sc->sc_lock);
	selrecord_knote(sip, kn);
	mutex_exit(&sc->sc_lock);

	error = 0;

out:	ugenif_release(sc);
	return error;
}

MODULE(MODULE_CLASS_DRIVER, ugen, NULL);

static int
ugen_modcmd(modcmd_t cmd, void *aux)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		mutex_init(&ugenif.lock, MUTEX_DEFAULT, IPL_NONE);
		rb_tree_init(&ugenif.tree, &ugenif_tree_ops);
		return 0;
	default:
		return ENOTTY;
	}
}
