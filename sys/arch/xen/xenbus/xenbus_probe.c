/* $NetBSD: xenbus_probe.c,v 1.37.2.2 2017/12/03 11:36:52 jdolecek Exp $ */
/******************************************************************************
 * Talks to Xen Store to figure out what devices we have.
 *
 * Copyright (C) 2005 Rusty Russell, IBM Corporation
 * Copyright (C) 2005 Mike Wray, Hewlett-Packard
 * Copyright (C) 2005 XenSource Ltd
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenbus_probe.c,v 1.37.2.2 2017/12/03 11:36:52 jdolecek Exp $");

#if 0
#define DPRINTK(fmt, args...) \
    printf("xenbus_probe (%s:%d) " fmt ".\n", __func__, __LINE__, ##args)
#else
#define DPRINTK(fmt, args...) ((void)0)
#endif

#include <sys/types.h>
#include <sys/null.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kthread.h>
#include <uvm/uvm.h>

#include <xen/xen.h>	/* for xendomain_is_dom0() */
#include <xen/hypervisor.h>
#include <xen/xenbus.h>
#include <xen/evtchn.h>
#include <xen/shutdown_xenbus.h>

#include "xenbus_comms.h"

extern struct semaphore xenwatch_mutex;

#define streq(a, b) (strcmp((a), (b)) == 0)

static int  xenbus_match(device_t, cfdata_t, void *);
static void xenbus_attach(device_t, device_t, void *);
static int  xenbus_print(void *, const char *);

/* power management, for save/restore */
static bool xenbus_suspend(device_t, const pmf_qual_t *);
static bool xenbus_resume(device_t, const pmf_qual_t *);

/* routines gathering device information from XenStore */
static int  read_otherend_details(struct xenbus_device *,
		const char *, const char *);
static int  read_backend_details (struct xenbus_device *);
static int  read_frontend_details(struct xenbus_device *);
static void free_otherend_details(struct xenbus_device *);

static int  watch_otherend     (struct xenbus_device *);
static void free_otherend_watch(struct xenbus_device *);

static void xenbus_probe_init(void *);

static struct xenbus_device *xenbus_lookup_device_path(const char *);

CFATTACH_DECL_NEW(xenbus, 0, xenbus_match, xenbus_attach,
    NULL, NULL);

device_t xenbus_dev;

SLIST_HEAD(, xenbus_device) xenbus_device_list;
SLIST_HEAD(, xenbus_backend_driver) xenbus_backend_driver_list =
	SLIST_HEAD_INITIALIZER(xenbus_backend_driver);

int
xenbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct xenbus_attach_args *xa = (struct xenbus_attach_args *)aux;

	if (strcmp(xa->xa_device, "xenbus") == 0)
		return 1;
	return 0;
}

static void
xenbus_attach(device_t parent, device_t self, void *aux)
{
	int err;

	aprint_normal(": Xen Virtual Bus Interface\n");
	xenbus_dev = self;
	config_pending_incr(self);

	err = kthread_create(PRI_NONE, 0, NULL, xenbus_probe_init, NULL,
	    NULL, "xenbus_probe");
	if (err)
		aprint_error_dev(xenbus_dev,
				"kthread_create(xenbus_probe): %d\n", err);

	if (!pmf_device_register(self, xenbus_suspend, xenbus_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static bool
xenbus_suspend(device_t dev, const pmf_qual_t *qual)
{
	xs_suspend();
	xb_suspend_comms(dev);

	return true;
}

static bool
xenbus_resume(device_t dev, const pmf_qual_t *qual)
{
	xb_init_comms(dev);
	xs_resume();

	return true;
}

/*
 * Suspend a xenbus device
 */
bool
xenbus_device_suspend(struct xenbus_device *dev) {

	free_otherend_details(dev);
	return true;
}

/*
 * Resume a xenbus device
 */
bool
xenbus_device_resume(struct xenbus_device *dev) {

	if (dev->xbusd_type == XENBUS_FRONTEND_DEVICE) {
		read_backend_details(dev);
	}

	return true;
}

void
xenbus_backend_register(struct xenbus_backend_driver *xbakd)
{
	SLIST_INSERT_HEAD(&xenbus_backend_driver_list, xbakd, xbakd_entries);
}

static int
read_otherend_details(struct xenbus_device *xendev,
				 const char *id_node, const char *path_node)
{
	int err;
	char *val, *ep;

	err = xenbus_read(NULL, xendev->xbusd_path, id_node, NULL, &val);
	if (err) {
		printf("reading other end details %s from %s\n",
		    id_node, xendev->xbusd_path);
		xenbus_dev_fatal(xendev, err,
				 "reading other end details %s from %s",
				 id_node, xendev->xbusd_path);
		return err;
	}
	xendev->xbusd_otherend_id = strtoul(val, &ep, 10);
	if (val[0] == '\0' || *ep != '\0') {
		printf("reading other end details %s from %s: %s is not a number\n", id_node, xendev->xbusd_path, val);
		xenbus_dev_fatal(xendev, err,
		    "reading other end details %s from %s: %s is not a number",
		    id_node, xendev->xbusd_path, val);
		free(val, M_DEVBUF);
		return EFTYPE;
	}
	free(val, M_DEVBUF);
	err = xenbus_read(NULL, xendev->xbusd_path, path_node, NULL, &val);
	if (err) {
		printf("reading other end details %s from %s (%d)\n",
		    path_node, xendev->xbusd_path, err);
		xenbus_dev_fatal(xendev, err,
				 "reading other end details %s from %s",
				 path_node, xendev->xbusd_path);
		return err;
	}
	DPRINTK("read_otherend_details: read %s/%s returned %s\n",
	    xendev->xbusd_path, path_node, val);
	xendev->xbusd_otherend = val;

	if (strlen(xendev->xbusd_otherend) == 0 ||
	    !xenbus_exists(NULL, xendev->xbusd_otherend, "")) {
		printf("missing other end from %s\n", xendev->xbusd_path);
		xenbus_dev_fatal(xendev, -ENOENT, "missing other end from %s",
				 xendev->xbusd_path);
		free_otherend_details(xendev);
		return ENOENT;
	}

	return 0;
}

static int
read_backend_details(struct xenbus_device *xendev)
{
	return read_otherend_details(xendev, "backend-id", "backend");
}


static int
read_frontend_details(struct xenbus_device *xendev)
{
	return read_otherend_details(xendev, "frontend-id", "frontend");
}

static void
free_otherend_details(struct xenbus_device *dev)
{
	free(dev->xbusd_otherend, M_DEVBUF);
	dev->xbusd_otherend = NULL;
}

static void
free_otherend_watch(struct xenbus_device *dev)
{
	if (dev->xbusd_otherend_watch.node) {
		unregister_xenbus_watch(&dev->xbusd_otherend_watch);
		free(dev->xbusd_otherend_watch.node, M_DEVBUF);
		dev->xbusd_otherend_watch.node = NULL;
	}
}

static void
otherend_changed(struct xenbus_watch *watch,
			     const char **vec, unsigned int len)
{
	struct xenbus_device *xdev = watch->xbw_dev;
	XenbusState state;

	/* Protect us against watches firing on old details when the otherend
	   details change, say immediately after a resume. */
	if (!xdev->xbusd_otherend ||
	    strncmp(xdev->xbusd_otherend, vec[XS_WATCH_PATH],
		    strlen(xdev->xbusd_otherend))) {
		DPRINTK("Ignoring watch at %s", vec[XS_WATCH_PATH]);
		return;
	}

	state = xenbus_read_driver_state(xdev->xbusd_otherend);

	DPRINTK("state is %d, %s, %s",
		state, xdev->xbusd_otherend_watch.node, vec[XS_WATCH_PATH]);
	if (state == XenbusStateClosed) {
		int error;
		if (xdev->xbusd_type == XENBUS_BACKEND_DEVICE) {
			error = xdev->xbusd_u.b.b_detach(
			    xdev->xbusd_u.b.b_cookie);
			if (error) {
				printf("could not detach %s: %d\n",
				    xdev->xbusd_path, error);
				return;
			}
		} else {
			error = config_detach(xdev->xbusd_u.f.f_dev,
			    DETACH_FORCE);
			if (error) {
				printf("could not detach %s: %d\n",
				    device_xname(xdev->xbusd_u.f.f_dev), error);
				return;
			}
		}
		xenbus_free_device(xdev);
		return;
	}
	if (xdev->xbusd_otherend_changed)
		xdev->xbusd_otherend_changed(
		    (xdev->xbusd_type == XENBUS_BACKEND_DEVICE) ?
		    xdev->xbusd_u.b.b_cookie : xdev->xbusd_u.f.f_dev, state);
}

static int
watch_otherend(struct xenbus_device *dev)
{
	free_otherend_watch(dev);

	return xenbus_watch_path2(dev, dev->xbusd_otherend, "state",
				  &dev->xbusd_otherend_watch,
				  otherend_changed);
}

static struct xenbus_device *
xenbus_lookup_device_path(const char *path)
{
	struct xenbus_device *xbusd;

	SLIST_FOREACH(xbusd, &xenbus_device_list, xbusd_entries) {
		if (strcmp(xbusd->xbusd_path, path) == 0)
			return xbusd;
	}
	return NULL;
}

static int
xenbus_probe_device_type(const char *path, const char *type,
    int (*create)(struct xenbus_device *))
{
	int err, i, pos, msize;
	int *lookup = NULL;
	unsigned long state;
	char **dir;
	unsigned int dir_n = 0;
	struct xenbus_device *xbusd;
	struct xenbusdev_attach_args xa;
	char *ep;

	DPRINTK("probe %s type %s", path, type);
	err = xenbus_directory(NULL, path, "", &dir_n, &dir);
	DPRINTK("directory err %d dir_n %d", err, dir_n);
	if (err)
		return err;

	/* Only sort frontend devices i.e. create == NULL*/
	if (dir_n > 1 && create == NULL) {
		int minp;
		unsigned long minv;
		unsigned long *id;

		lookup = malloc(sizeof(int) * dir_n, M_DEVBUF,
		    M_WAITOK | M_ZERO);
		if (lookup == NULL)
			panic("can't malloc lookup");

		id = malloc(sizeof(unsigned long) * dir_n, M_DEVBUF,
		    M_WAITOK | M_ZERO);
		if (id == NULL)
			panic("can't malloc id");

		/* Convert string values to numeric; skip invalid */
		for (i = 0; i < dir_n; i++) {
			/*
			 * Add one to differentiate numerical zero from invalid
			 * string. Has no effect on sort order.
			 */
			id[i] = strtoul(dir[i], &ep, 10) + 1;
			if (dir[i][0] == '\0' || *ep != '\0')
				id[i] = 0;
		}
		
		/* Build lookup table in ascending order */
		for (pos = 0; pos < dir_n; ) {
			minv = UINT32_MAX;
			minp = -1;
			for (i = 0; i < dir_n; i++) {
				if (id[i] < minv && id[i] > 0) {
					minv = id[i];
					minp = i;
				}
			}
			if (minp >= 0) {
				lookup[pos++] = minp;
				id[minp] = 0;
			}
			else
				break;
		}
		
		free(id, M_DEVBUF);
		/* Adjust in case we had to skip non-numeric entries */
		dir_n = pos;
	}

	for (pos = 0; pos < dir_n; pos++) {
		err = 0;
		if (lookup)
			i = lookup[pos];
		else
			i = pos;
		/*
		 * add size of path to size of xenbus_device. xenbus_device
		 * already has room for one char in xbusd_path.
		 */
		msize = sizeof(*xbusd) + strlen(path) + strlen(dir[i]) + 2;
		xbusd = malloc(msize, M_DEVBUF, M_WAITOK | M_ZERO);
		if (xbusd == NULL)
			panic("can't malloc xbusd");
			
		snprintf(__UNCONST(xbusd->xbusd_path),
		    msize - sizeof(*xbusd) + 1, "%s/%s", path, dir[i]);
		if (xenbus_lookup_device_path(xbusd->xbusd_path) != NULL) {
			/* device already registered */
			free(xbusd, M_DEVBUF);
			continue;
		}
		err = xenbus_read_ul(NULL, xbusd->xbusd_path, "state",
		    &state, 10);
		if (err) {
			printf("xenbus: can't get state "
			    "for %s (%d)\n", xbusd->xbusd_path, err);
			free(xbusd, M_DEVBUF);
			err = 0;
			continue;
		}
		if (state != XenbusStateInitialising) {
			/* device is not new */
			free(xbusd, M_DEVBUF);
			continue;
		}

		xbusd->xbusd_otherend_watch.xbw_dev = xbusd;
		DPRINTK("xenbus_probe_device_type probe %s\n",
		    xbusd->xbusd_path);
		if (create != NULL) {
			xbusd->xbusd_type = XENBUS_BACKEND_DEVICE;
			err = read_frontend_details(xbusd);
			if (err != 0) {
				printf("xenbus: can't get frontend details "
				    "for %s (%d)\n", xbusd->xbusd_path, err);
				break;
			}
			if (create(xbusd)) {
				free(xbusd, M_DEVBUF);
				continue;
			}
		} else {
			xbusd->xbusd_type = XENBUS_FRONTEND_DEVICE;
			xa.xa_xbusd = xbusd;
			xa.xa_type = type;
			xa.xa_id = strtoul(dir[i], &ep, 0);
			if (dir[i][0] == '\0' || *ep != '\0') {
				printf("xenbus device type %s: id %s is not a"
				    " number\n", type, dir[i]);
				err = EFTYPE;
				free(xbusd, M_DEVBUF);
				break;
			}
			err = read_backend_details(xbusd);
			if (err != 0) {
				printf("xenbus: can't get backend details "
				    "for %s (%d)\n", xbusd->xbusd_path, err);
				break;
			}
			xbusd->xbusd_u.f.f_dev = config_found_ia(xenbus_dev,
			    "xenbus", &xa, xenbus_print);
			if (xbusd->xbusd_u.f.f_dev == NULL) {
				free(xbusd, M_DEVBUF);
				continue;
			}
		}
		SLIST_INSERT_HEAD(&xenbus_device_list,
		    xbusd, xbusd_entries);
		watch_otherend(xbusd);
	}
	free(dir, M_DEVBUF);
	if (lookup)
		free(lookup, M_DEVBUF);
	
	return err;
}

static int
xenbus_print(void *aux, const char *pnp)
{
	struct xenbusdev_attach_args *xa = aux;

	if (pnp) {
		if (strcmp(xa->xa_type, "vbd") == 0)
			aprint_normal("xbd");
		else if (strcmp(xa->xa_type, "vif") == 0)
			aprint_normal("xennet");
		else if (strcmp(xa->xa_type, "balloon") == 0)
			aprint_normal("balloon");
		else
			aprint_normal("unknown type %s", xa->xa_type);
		aprint_normal(" at %s", pnp);
	}
	aprint_normal(" id %d", xa->xa_id);
	return(UNCONF);
}

static int
xenbus_probe_frontends(void)
{
	int err;
	char **dir;
	unsigned int i, dir_n;
	char path[30];

	DPRINTK("probe device");
	err = xenbus_directory(NULL, "device", "", &dir_n, &dir);
	DPRINTK("directory err %d dir_n %d", err, dir_n);
	if (err)
		return err;

	for (i = 0; i < dir_n; i++) {
		/*
		 * console is configured through xen_start_info when
		 * xencons is attaching to hypervisor, so avoid console
		 * probing when configuring xenbus devices
		 */
		if (strcmp(dir[i], "console") == 0)
			continue;

		snprintf(path, sizeof(path), "device/%s", dir[i]);
		err = xenbus_probe_device_type(path, dir[i], NULL);
		if (err)
			break;
	}
	free(dir, M_DEVBUF);
	return err;
}

static int
xenbus_probe_backends(void)
{
	int err;
	char **dirt, **dirid;
	unsigned int type, id, dirt_n, dirid_n;
	char path[30];
	struct xenbus_backend_driver *xbakd;

	DPRINTK("probe backend");
	err = xenbus_directory(NULL, "backend", "", &dirt_n, &dirt);
	DPRINTK("directory err %d dirt_n %d", err, dirt_n);
	if (err)
		return err;

	for (type = 0; type < dirt_n; type++) {
		SLIST_FOREACH(xbakd, &xenbus_backend_driver_list,
		    xbakd_entries) {
			if (strcmp(dirt[type], xbakd->xbakd_type) == 0)
				break;
		}
		if (xbakd == NULL)
			continue;
		err = xenbus_directory(NULL, "backend", dirt[type],
		    &dirid_n, &dirid);
		DPRINTK("directory backend/%s err %d dirid_n %d",
		    dirt[type], err, dirid_n);
		if (err) {
			free(dirt, M_DEVBUF); /* to be checked */
			return err;
		}
		for (id = 0; id < dirid_n; id++) {
			snprintf(path, sizeof(path), "backend/%s/%s",
			    dirt[type], dirid[id]);
			err = xenbus_probe_device_type(path, dirt[type],
			    xbakd->xbakd_create);
			if (err)
				break;
		}
		free(dirid, M_DEVBUF);
	}
	free(dirt, M_DEVBUF);
	return err;
}

int
xenbus_free_device(struct xenbus_device *xbusd)
{
	KASSERT(xenbus_lookup_device_path(xbusd->xbusd_path) == xbusd);
	SLIST_REMOVE(&xenbus_device_list, xbusd, xenbus_device, xbusd_entries);
	free_otherend_watch(xbusd);
	free_otherend_details(xbusd);
	xenbus_switch_state(xbusd, NULL, XenbusStateClosed);
	free(xbusd, M_DEVBUF);
	return 0;
}

static void
frontend_changed(struct xenbus_watch *watch,
			     const char **vec, unsigned int len)
{
	DPRINTK("frontend_changed %s\n", vec[XS_WATCH_PATH]);
	xenbus_probe_frontends();
}

static void
backend_changed(struct xenbus_watch *watch,
			    const char **vec, unsigned int len)
{
	DPRINTK("backend_changed %s\n", vec[XS_WATCH_PATH]);
	xenbus_probe_backends();
}


/* We watch for devices appearing and vanishing. */
static struct xenbus_watch fe_watch;

static struct xenbus_watch be_watch;

/* A flag to determine if xenstored is 'ready' (i.e. has started) */
int xenstored_ready = 0;

void
xenbus_probe(void *unused)
{
	struct xenbusdev_attach_args balloon_xa = {
		.xa_id = 0,
		.xa_type = "balloon"
	};

	KASSERT((xenstored_ready > 0));

	/* Enumerate devices in xenstore. */
	xenbus_probe_frontends();
	xenbus_probe_backends();

	/* Watch for changes. */
	fe_watch.node = malloc(strlen("device" + 1), M_DEVBUF, M_NOWAIT);
	strcpy(fe_watch.node, "device");
	fe_watch.xbw_callback = frontend_changed;
	register_xenbus_watch(&fe_watch);
	be_watch.node = malloc(strlen("backend" + 1), M_DEVBUF, M_NOWAIT);
	strcpy(be_watch.node, "backend");
	be_watch.xbw_callback = backend_changed;
	register_xenbus_watch(&be_watch);

	/* attach balloon. */
	config_found_ia(xenbus_dev, "xenbus", &balloon_xa, xenbus_print);

	shutdown_xenbus_setup();

	/* Notify others that xenstore is up */
	//notifier_call_chain(&xenstore_chain, 0, NULL);
}

static void
xenbus_probe_init(void *unused)
{
	int err = 0;
	bool dom0;
	vaddr_t page = 0;

	DPRINTK("");

	SLIST_INIT(&xenbus_device_list);

	/*
	** Domain0 doesn't have a store_evtchn or store_mfn yet.
	*/
	dom0 = xendomain_is_dom0();
	if (dom0) {
#if defined(DOM0OPS)
		paddr_t ma;
		evtchn_op_t op = { .cmd = 0 };

		/* Allocate page. */
		page = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
		    UVM_KMF_ZERO | UVM_KMF_WIRED);
		if (!page) 
			panic("can't get xenstore page");

		(void)pmap_extract_ma(pmap_kernel(), page, &ma);
		xen_start_info.store_mfn = ma >> PAGE_SHIFT;
		xenstore_interface = (void *)page;
		
		/* Next allocate a local port which xenstored can bind to */
		op.cmd = EVTCHNOP_alloc_unbound;
		op.u.alloc_unbound.dom        = DOMID_SELF;
		op.u.alloc_unbound.remote_dom = 0;

		err = HYPERVISOR_event_channel_op(&op);
		if (err) {
			aprint_error_dev(xenbus_dev,
				"can't register xenstore event\n");
			goto err0;
		}
		
		xen_start_info.store_evtchn = op.u.alloc_unbound.port;

		DELAY(1000);
#else /* DOM0OPS */
		kthread_exit(0); /* can't get a working xenstore in this case */
#endif /* DOM0OPS */
	}

	/* Publish xenbus and Xenstore info in /kern/xen */
	xenbus_kernfs_init();

	/* register event handler */
	xb_init_comms(xenbus_dev);

	/* Initialize the interface to xenstore. */
	err = xs_init(xenbus_dev);
	if (err) {
		aprint_error_dev(xenbus_dev,
		    "Error initializing xenstore comms: %i\n", err);
		goto err0;
	}

	if (!dom0) {
		xenstored_ready = 1;
		xenbus_probe(NULL);
	}

	DPRINTK("done");
	config_pending_decr(xenbus_dev);
#ifdef DOM0OPS
	if (dom0) {
		int s;
		s = spltty();
		while (xenstored_ready == 0) {
			tsleep(&xenstored_ready, PRIBIO, "xsready", 0);
			xenbus_probe(NULL);
		}
		splx(s);
	}
#endif
	kthread_exit(0);

err0:
	if (page)
		uvm_km_free(kernel_map, page, PAGE_SIZE,
				UVM_KMF_ZERO | UVM_KMF_WIRED);
	kthread_exit(err);
}

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
