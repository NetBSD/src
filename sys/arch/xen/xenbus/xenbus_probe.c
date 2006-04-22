/* $NetBSD: xenbus_probe.c,v 1.9.2.2 2006/04/22 11:38:11 simonb Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: xenbus_probe.c,v 1.9.2.2 2006/04/22 11:38:11 simonb Exp $");

#if 0
#define DPRINTK(fmt, args...) \
    printf("xenbus_probe (%s:%d) " fmt ".\n", __FUNCTION__, __LINE__, ##args)
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

#include <machine/stdarg.h>

#include <machine/hypervisor.h>
#include <machine/xenbus.h>
#include <machine/evtchn.h>

#include "xenbus_comms.h"

extern struct semaphore xenwatch_mutex;

#define streq(a, b) (strcmp((a), (b)) == 0)

static int  xenbus_match(struct device *, struct cfdata *, void *);
static void xenbus_attach(struct device *, struct device *, void *);
static int  xenbus_print(void *, const char *);

static void xenbus_kthread_create(void *);
static void xenbus_probe_init(void *);

static struct xenbus_device *xenbus_lookup_device_path(const char *);

CFATTACH_DECL(xenbus, sizeof(struct device), xenbus_match, xenbus_attach,
    NULL, NULL);

struct device *xenbus_sc;

SLIST_HEAD(, xenbus_device) xenbus_device_list;

int
xenbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct xenbus_attach_args *xa = (struct xenbus_attach_args *)aux;

	if (strcmp(xa->xa_device, "xenbus") == 0)
		return 1;
	return 0;
}

static void
xenbus_attach(struct device *parent, struct device *self, void *aux)
{
	aprint_normal(": Xen Virtual Bus Interface\n");
	xenbus_sc = self;
	xb_init_comms(self);
	config_pending_incr();
	kthread_create(xenbus_kthread_create, NULL);
}

void
xenbus_kthread_create(void *unused)
{
	struct proc *p;
	int err;
	err = kthread_create1(xenbus_probe_init, NULL, &p, "xenbus_probe");
	if (err)
		printf("kthread_create1(xenbus_probe): %d\n", err);
}


#if 0
static char *kasprintf(const char *fmt, ...);

static struct notifier_block *xenstore_chain;

/* If something in array of ids matches this device, return it. */
static const struct xenbus_device_id *
match_device(const struct xenbus_device_id *arr, struct xenbus_device *dev)
{
	for (; !streq(arr->devicetype, ""); arr++) {
		if (streq(arr->devicetype, dev->devicetype))
			return arr;
	}
	return NULL;
}

static int xenbus_match_device(struct device *_dev, struct device_driver *_drv)
{
	struct xenbus_driver *drv = to_xenbus_driver(_drv);

	if (!drv->ids)
		return 0;

	return match_device(drv->ids, to_xenbus_device(_dev)) != NULL;
}
#endif

struct xen_bus_type
{
	const char *root;
	unsigned int levels;
};

#if 0
/* device/<type>/<id> => <type>-<id> */
static int
frontend_bus_id(char bus_id[BUS_ID_SIZE], const char *nodename)
{
	nodename = strchr(nodename, '/');
	if (!nodename || strlen(nodename + 1) >= BUS_ID_SIZE) {
		printk(KERN_WARNING "XENBUS: bad frontend %s\n", nodename);
		return EINVAL;
	}

	strlcpy(bus_id, nodename + 1, BUS_ID_SIZE);
	if (!strchr(bus_id, '/')) {
		printk(KERN_WARNING "XENBUS: bus_id %s no slash\n", bus_id);
		return EINVAL;
	}
	*strchr(bus_id, '/') = '-';
	return 0;
}
#endif

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
		free(xendev->xbusd_otherend, M_DEVBUF);
		xendev->xbusd_otherend = NULL;
		return ENOENT;
	}

	return 0;
}

static int
read_backend_details(struct xenbus_device *xendev)
{
	return read_otherend_details(xendev, "backend-id", "backend");
}


#if 0 //#ifdef DOM0OPS
static int
read_frontend_details(struct xenbus_device *xendev)
{
	return read_otherend_details(xendev, "frontend-id", "frontend");
}
#endif

#if unused
static void
free_otherend_details(struct xenbus_device *dev)
{
	free(dev->xbusd_otherend, M_DEVBUF);
	dev->xbusd_otherend = NULL;
}
#endif


static void
free_otherend_watch(struct xenbus_device *dev)
{
	if (dev->xbusd_otherend_watch.node) {
		unregister_xenbus_watch(&dev->xbusd_otherend_watch);
		free(dev->xbusd_otherend_watch.node, M_DEVBUF);
		dev->xbusd_otherend_watch.node = NULL;
	}
}

#if 0

/* Bus type for frontend drivers. */
static int xenbus_probe_frontend(const char *type, const char *name);
#endif
static struct xen_bus_type xenbus_frontend = {
	.root = "device",
	.levels = 2, 		/* device/type/<id> */
};

#if 0
/* backend/<type>/<fe-uuid>/<id> => <type>-<fe-domid>-<id> */
static int
backend_bus_id(char bus_id[BUS_ID_SIZE], const char *nodename)
{
	int domid, err;
	const char *devid, *type, *frontend;
	unsigned int typelen;

	type = strchr(nodename, '/');
	if (!type)
		return EINVAL;
	type++;
	typelen = strcspn(type, "/");
	if (!typelen || type[typelen] != '/')
		return EINVAL;

	devid = strrchr(nodename, '/') + 1;

	err = xenbus_gather(NULL, nodename, "frontend-id", "%i", &domid,
			    "frontend", NULL, &frontend,
			    NULL);
	if (err)
		return err;
	if (strlen(frontend) == 0)
		err = ERANGE;

	if (!err && !xenbus_exists(NULL, frontend, ""))
		err = ENOENT;

	if (err) {
		free(frontend, M_DEVBUF);
		return err;
	}

	if (snprintf(bus_id, BUS_ID_SIZE,
		     "%.*s-%i-%s", typelen, type, domid, devid) >= BUS_ID_SIZE)
		return ENOSPC;
	return 0;
}

static int
xenbus_hotplug_backend(struct device *dev, char **envp,
				  int num_envp, char *buffer, int buffer_size)
{
	struct xenbus_device *xdev;
	struct xenbus_driver *drv = NULL;
	int i = 0;
	int length = 0;
	char *basepath_end;
	char *frontend_id;

	DPRINTK("");

	if (dev == NULL)
		return ENODEV;

	xdev = to_xenbus_device(dev);
	if (xdev == NULL)
		return ENODEV;

	if (dev->driver)
		drv = to_xenbus_driver(dev->driver);

	/* stuff we want to pass to /sbin/hotplug */
	add_hotplug_env_var(envp, num_envp, &i,
			    buffer, buffer_size, &length,
			    "XENBUS_TYPE=%s", xdev->devicetype);

	add_hotplug_env_var(envp, num_envp, &i,
			    buffer, buffer_size, &length,
			    "XENBUS_PATH=%s", xdev->xbusd_path);

	add_hotplug_env_var(envp, num_envp, &i,
			    buffer, buffer_size, &length,
			    "XENBUS_BASE_PATH=%s", xdev->xbusd_path);

	basepath_end = strrchr(envp[i - 1], '/');
	length -= strlen(basepath_end);
	*basepath_end = '\0';
	basepath_end = strrchr(envp[i - 1], '/');
	length -= strlen(basepath_end);
	*basepath_end = '\0';

	basepath_end++;
	frontend_id = kmalloc(strlen(basepath_end) + 1, GFP_KERNEL);
	strcpy(frontend_id, basepath_end);
	add_hotplug_env_var(envp, num_envp, &i,
			    buffer, buffer_size, &length,
			    "XENBUS_FRONTEND_ID=%s", frontend_id);
	free(frontend_id, M_DEVBUF);

	/* terminate, set to next free slot, shrink available space */
	envp[i] = NULL;
	envp = &envp[i];
	num_envp -= i;
	buffer = &buffer[length];
	buffer_size -= length;

	if (drv && drv->hotplug)
		return drv->hotplug(xdev, envp, num_envp, buffer,
				    buffer_size);

	return 0;
}

static int xenbus_probe_backend(const char *type, const char *domid);
#endif

static struct xen_bus_type xenbus_backend = {
	.root = "backend",
	.levels = 3, 		/* backend/type/<frontend>/<id> */
};

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
		error = config_detach(xdev->xbusd_dev, DETACH_FORCE);
		if (error) {
			printf("could not detach %s: %d\n",
			    xdev->xbusd_dev->dv_xname, error);
			return;
		}
		xenbus_free_device(xdev);
		return;
	}
	if (xdev->xbusd_otherend_changed)
		xdev->xbusd_otherend_changed(xdev->xbusd_dev, state);
}

static int
talk_to_otherend(struct xenbus_device *dev)
{
	free_otherend_watch(dev);

	return xenbus_watch_path2(dev, dev->xbusd_otherend, "state",
				  &dev->xbusd_otherend_watch,
				  otherend_changed);
}

#if 0

static int
xenbus_dev_probe(struct device *_dev)
{
	struct xenbus_device *dev = to_xenbus_device(_dev);
	struct xenbus_driver *drv = to_xenbus_driver(_dev->driver);
	const struct xenbus_device_id *id;
	int err;

	DPRINTK("");

	err = talk_to_otherend(dev);
	if (err) {
		printk(KERN_WARNING
		       "xenbus_probe: talk_to_otherend on %s failed.\n",
		       dev->xbusd_path);
		return err;
	}

	if (!drv->probe) {
		err = ENODEV;
		goto fail;
	}

	id = match_device(drv->ids, dev);
	if (!id) {
		err = ENODEV;
		goto fail;
	}

	err = drv->probe(dev, id);
	if (err)
		goto fail;

	return 0;
fail:
	xenbus_dev_error(dev, err, "xenbus_dev_probe on %s", dev->xbusd_path);
	xenbus_switch_state(dev, NULL, XenbusStateClosed);
	return ENODEV;
	
}

static int
xenbus_dev_remove(struct device *_dev)
{
	struct xenbus_device *dev = to_xenbus_device(_dev);
	struct xenbus_driver *drv = to_xenbus_driver(_dev->driver);

	DPRINTK("");

	free_otherend_watch(dev);
	free_otherend_details(dev);

	if (drv->remove)
		drv->remove(dev);

	xenbus_switch_state(dev, NULL, XenbusStateClosed);
	return 0;
}

static int
xenbus_register_driver_common(struct xenbus_driver *drv,
					 struct xen_bus_type *bus)
{
	int ret;

	drv->driver.name = drv->name;
	drv->driver.bus = &bus->bus;
	drv->driver.owner = drv->owner;
	drv->driver.probe = xenbus_dev_probe;
	drv->driver.remove = xenbus_dev_remove;

	down(&xenwatch_mutex);
	ret = driver_register(&drv->driver);
	up(&xenwatch_mutex);
	return ret;
}

int
xenbus_register_frontend(struct xenbus_driver *drv)
{
	drv->read_otherend_details = read_backend_details;

	return xenbus_register_driver_common(drv, &xenbus_frontend);
}

int
xenbus_register_backend(struct xenbus_driver *drv)
{
	drv->read_otherend_details = read_frontend_details;

	return xenbus_register_driver_common(drv, &xenbus_backend);
}

void
xenbus_unregister_driver(struct xenbus_driver *drv)
{
	driver_unregister(&drv->driver);
}

struct xb_find_info
{
	struct xenbus_device *dev;
	const char *nodename;
};

static int
cmp_dev(struct device *dev, void *data)
{
	struct xenbus_device *xendev = to_xenbus_device(dev);
	struct xb_find_info *info = data;

	if (streq(xendev->xbusd_path, info->nodename)) {
		info->dev = xendev;
		get_device(dev);
		return 1;
	}
	return 0;
}

struct xenbus_device *
xenbus_device_find(const char *nodename, struct bus_type *bus)
{
	struct xb_find_info info = { .dev = NULL, .nodename = nodename };

	bus_for_each_dev(bus, NULL, &info, cmp_dev);
	return info.dev;
}

static int
cleanup_dev(struct device *dev, void *data)
{
	struct xenbus_device *xendev = to_xenbus_device(dev);
	struct xb_find_info *info = data;
	int len = strlen(info->nodename);

	DPRINTK("%s", info->nodename);

	if (!strncmp(xendev->xbusd_path, info->nodename, len)) {
		info->dev = xendev;
		get_device(dev);
		return 1;
	}
	return 0;
}

static void
xenbus_cleanup_devices(const char *path, struct bus_type *bus)
{
	struct xb_find_info info = { .nodename = path };

	do {
		info.dev = NULL;
		bus_for_each_dev(bus, NULL, &info, cleanup_dev);
		if (info.dev) {
			device_unregister(&info.dev->dev);
			put_device(&info.dev->dev);
		}
	} while (info.dev);
}

static void
xenbus_dev_free(struct xenbus_device *xendev)
{
	free(xendev, M_DEVBUF);
}

static void
xenbus_dev_release(struct device *dev)
{
	if (dev) {
		xenbus_dev_free(to_xenbus_device(dev));
	}
}

/* Simplified asprintf. */
static char *kasprintf(const char *fmt, ...)
{
	va_list ap;
	unsigned int len;
	char *p, dummy[1];

	va_start(ap, fmt);
	/* FIXME: vsnprintf has a bug, NULL should work */
	len = vsnprintf(dummy, 0, fmt, ap);
	va_end(ap);

	p = malloc(len + 1, M_DEVBUF, M_NOWAIT);
	if (!p)
		return NULL;
	va_start(ap, fmt);
	vsprintf(p, fmt, ap);
	va_end(ap);
	return p;
}
#endif

#if 0
static ssize_t
xendev_show_nodename(struct device *dev, char *buf)
{
	return sprintf(buf, "%s\n", to_xenbus_device(dev)->nodename);
}
// XXX implement
DEVICE_ATTR(nodename, S_IRUSR | S_IRGRP | S_IROTH, xendev_show_nodename, NULL);

static ssize_t
xendev_show_devtype(struct device *dev, char *buf)
{
	return sprintf(buf, "%s\n", to_xenbus_device(dev)->devicetype);
}
DEVICE_ATTR(devtype, S_IRUSR | S_IRGRP | S_IROTH, xendev_show_devtype, NULL);


static int
xenbus_probe_node(struct xen_bus_type *bus,
			     const char *type,
			     const char *nodename)
{
#define CHECK_FAIL				\
	do {					\
		if (err)			\
			goto fail;		\
	}					\
	while (0)				\


	struct xenbus_device *xendev;
	size_t stringlen;
	char *tmpstring;

	XenbusState state = xenbus_read_driver_state(nodename);

	printf("xenbus_probe_node %s %s %s %d\n", bus->root, type, nodename, state);
	if (state != XenbusStateInitialising) {
		/* Device is not new, so ignore it.  This can happen if a
		   device is going away after switching to Closed.  */
		return 0;
	}


	stringlen = strlen(nodename) + 1 + strlen(type) + 1;
	xendev = malloc(sizeof(*xendev) + stringlen, M_DEVBUF, M_NOWAIT);
	if (!xendev)
		return ENOMEM;
	memset(xendev, 0, sizeof(*xendev));

	/* Copy the strings into the extra space. */

	tmpstring = (char *)(xendev + 1);
	strcpy(tmpstring, nodename);
	xendev->nodename = tmpstring;

	tmpstring += strlen(tmpstring) + 1;
	strcpy(tmpstring, type);
	xendev->devicetype = tmpstring;

#if 0
	xendev->dev.parent = &bus->dev;
	xendev->dev.bus = &bus->bus;
	xendev->dev.release = xenbus_dev_release;

	err = bus->get_bus_id(xendev->dev.bus_id, xendev->nodename);
	CHECK_FAIL;

	/* Register with generic device framework. */
	err = device_register(&xendev->dev);
	CHECK_FAIL;

	device_create_file(&xendev->dev, &dev_attr_nodename);
	device_create_file(&xendev->dev, &dev_attr_devtype);

#endif
	return 0;

#undef CHECK_FAIL
#if 0
fail:
	xenbus_dev_free(xendev);
	return err;
#endif
}
#endif

#if 0
/* device/<typename>/<name> */
static int
xenbus_probe_frontend(const char *type, const char *name)
{
	char *nodename;
	int err;

	nodename = kasprintf("%s/%s/%s", xenbus_frontend.root, type, name);
	if (!nodename)
		return ENOMEM;
	
	DPRINTK("%s", nodename);

	err = xenbus_probe_node(&xenbus_frontend, type, nodename);
	free(nodename, M_DEVBUF);
	return err;
}

/* backend/<typename>/<frontend-uuid>/<name> */
static int
xenbus_probe_backend_unit(const char *dir,
				     const char *type,
				     const char *name)
{
	char *nodename;
	int err;

	nodename = kasprintf("%s/%s", dir, name);
	if (!nodename)
		return ENOMEM;

	DPRINTK("%s", nodename);

	err = xenbus_probe_node(&xenbus_backend, type, nodename);
	free(nodename, M_DEVBUF);
	return err;
}

/* backend/<typename>/<frontend-domid> */
static int
xenbus_probe_backend(const char *type, const char *domid)
{
	char *nodename;
	int err;
	char **dir;
	unsigned int i, dir_n = 0;

	DPRINTK("");

	nodename = kasprintf("%s/%s/%s", xenbus_backend.root, type, domid);
	if (!nodename)
		return ENOMEM;

	err = xenbus_directory(NULL, nodename, "", &dir_n, &dir);
	DPRINTK("xenbus_probe_backend err %d dir_n %d", err, dir_n);
	if (err) {
		free(nodename, M_DEVBUF);
		return err;
	}

	for (i = 0; i < dir_n; i++) {
		err = xenbus_probe_backend_unit(nodename, type, dir[i]);
		if (err)
			break;
	}
	free(dir, M_DEVBUF);
	free(nodename, M_DEVBUF);
	return err;
}
#endif

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
xenbus_probe_device_type(struct xen_bus_type *bus, const char *type)
{
	int err;
	char **dir;
	unsigned int dir_n = 0;
	int i;
	struct xenbus_device *xbusd;
	struct xenbusdev_attach_args xa;
	char *ep;

	DPRINTK("probe %s type %s", bus->root, type);
	err = xenbus_directory(NULL, bus->root, type, &dir_n, &dir);
	DPRINTK("directory err %d dir_n %d", err, dir_n);
	if (err)
		return err;

	for (i = 0; i < dir_n; i++) {
		int msize;
		/*
		 * add size of path to size of xenbus_device. xenbus_device
		 * already has room for one char in xbusd_path.
		 */
		msize = sizeof(*xbusd) + strlen(bus->root) + strlen(type)
		    + strlen(dir[i]) + 2;
		xbusd = malloc(msize, M_DEVBUF, M_WAITOK | M_ZERO);
		if (xbusd == NULL)
			panic("can't malloc xbusd");
			
		snprintf(__UNCONST(xbusd->xbusd_path),
		    msize - sizeof(*xbusd) + 1, "%s/%s/%s",
		    bus->root, type, dir[i]);
		if (xenbus_lookup_device_path(xbusd->xbusd_path) != NULL) {
			/* device already registered */
			free(xbusd, M_DEVBUF);
			continue;
		}

		xbusd->xbusd_otherend_watch.xbw_dev = xbusd;
		DPRINTK("xenbus_probe_device_type probe %s\n",
		    xbusd->xbusd_path);
		xa.xa_xbusd = xbusd;
		xa.xa_type = type;
		xa.xa_id = strtoul(dir[i], &ep, 0);
		if (dir[i][0] == '\0' || *ep != '\0') {
			printf("xenbus device type %s: id %s is not a number\n",
			    type, dir[i]);
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
		xbusd->xbusd_dev = config_found_ia(xenbus_sc, "xenbus", &xa,
		    xenbus_print);
		if (xbusd->xbusd_dev == NULL)
			free(xbusd, M_DEVBUF);
		else {
			SLIST_INSERT_HEAD(&xenbus_device_list,
			    xbusd, xbusd_entries);
			talk_to_otherend(xbusd);
		}
	}
	free(dir, M_DEVBUF);
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
		else
			aprint_normal("unknown type %s", xa->xa_type);
		aprint_normal(" at %s", pnp);
	}
	aprint_normal(" id %d", xa->xa_id);
	return(UNCONF);
}

static int
xenbus_probe_devices(struct xen_bus_type *bus)
{
	int err;
	char **dir;
	unsigned int i, dir_n;

	DPRINTK("probe %s", bus->root);
	err = xenbus_directory(NULL, bus->root, "", &dir_n, &dir);
	DPRINTK("directory err %d dir_n %d", err, dir_n);
	if (err)
		return err;

	for (i = 0; i < dir_n; i++) {
		err = xenbus_probe_device_type(bus, dir[i]);
		if (err)
			break;
	}
	free(dir, M_DEVBUF);
	return err;
}

int
xenbus_free_device(struct xenbus_device *xbusd)
{
	KASSERT(xenbus_lookup_device_path(xbusd->xbusd_path) == xbusd);
	SLIST_REMOVE(&xenbus_device_list, xbusd, xenbus_device, xbusd_entries);
	free_otherend_watch(xbusd);
	free(xbusd->xbusd_otherend, M_DEVBUF);
	xenbus_switch_state(xbusd, NULL, XenbusStateClosed);
	free(xbusd, M_DEVBUF);
	return 0;
}

#if 0
static unsigned int
char_count(const char *str, char c)
{
	unsigned int i, ret = 0;

	for (i = 0; str[i]; i++)
		if (str[i] == c)
			ret++;
	return ret;
}

static int
strsep_len(const char *str, char c, unsigned int len)
{
	unsigned int i;

	for (i = 0; str[i]; i++)
		if (str[i] == c) {
			if (len == 0)
				return i;
			len--;
		}
	return (len == 0) ? i : -ERANGE;
}

static void
dev_changed(const char *node, struct xen_bus_type *bus)
{
	int exists, rootlen;
	struct xenbus_device *dev;
	char type[BUS_ID_SIZE];
	const char *p, *root;

	if (char_count(node, '/') < 2)
 		return;

	exists = xenbus_exists(NULL, node, "");
	if (!exists) {
		xenbus_cleanup_devices(node, &bus->bus);
		return;
	}

	/* backend/<type>/... or device/<type>/... */
	p = strchr(node, '/') + 1;
	snprintf(type, BUS_ID_SIZE, "%.*s", (int)strcspn(p, "/"), p);
	type[BUS_ID_SIZE-1] = '\0';

	rootlen = strsep_len(node, '/', bus->levels);
	if (rootlen < 0)
		return;
	root = kasprintf("%.*s", rootlen, node);
	if (!root)
		return;

	dev = xenbus_device_find(root, &bus->bus);
	if (!dev)
		xenbus_probe_node(bus, type, root);
	else
		put_device(&dev->dev);

	free(root, M_DEVBUF);
}
#endif

static void
frontend_changed(struct xenbus_watch *watch,
			     const char **vec, unsigned int len)
{
	DPRINTK("");
	//printf("frontend_changed %s\n", vec[XS_WATCH_PATH]);
	xenbus_probe_devices(&xenbus_frontend);
}

static void
backend_changed(struct xenbus_watch *watch,
			    const char **vec, unsigned int len)
{
	DPRINTK("");

	printf("backend_changed %s\n", vec[XS_WATCH_PATH]);
	//dev_changed(vec[XS_WATCH_PATH], &xenbus_backend);
}


/* We watch for devices appearing and vanishing. */
static struct xenbus_watch fe_watch;

static struct xenbus_watch be_watch;

#if 0
static int suspend_dev(struct device *dev, void *data)
{
	int err = 0;
	struct xenbus_driver *drv;
	struct xenbus_device *xdev;

	DPRINTK("");

	if (dev->driver == NULL)
		return 0;
	drv = to_xenbus_driver(dev->driver);
	xdev = container_of(dev, struct xenbus_device, dev);
	if (drv->suspend)
		err = drv->suspend(xdev);
	if (err)
		printk(KERN_WARNING
		       "xenbus: suspend %s failed: %i\n", dev->bus_id, err);
	return 0;
}

static int
resume_dev(struct device *dev, void *data)
{
	int err;
	struct xenbus_driver *drv;
	struct xenbus_device *xdev;

	DPRINTK("");

	if (dev->driver == NULL)
		return 0;
	drv = to_xenbus_driver(dev->driver);
	xdev = container_of(dev, struct xenbus_device, dev);

	err = talk_to_otherend(xdev);
	if (err) {
		printk(KERN_WARNING
		       "xenbus: resume (talk_to_otherend) %s failed: %i\n",
		       dev->bus_id, err);
		return err;
	}

	if (drv->resume)
		err = drv->resume(xdev);
	if (err)
		printk(KERN_WARNING
		       "xenbus: resume %s failed: %i\n", dev->bus_id, err);
	return err;
}

void
xenbus_suspend(void)
{
	DPRINTK("");

	bus_for_each_dev(&xenbus_frontend.bus, NULL, NULL, suspend_dev);
	bus_for_each_dev(&xenbus_backend.bus, NULL, NULL, suspend_dev);
	xs_suspend();
}

void xenbus_resume(struct device *dev)
{
	xb_init_comms(dev);
	xs_resume();
	bus_for_each_dev(&xenbus_frontend.bus, NULL, NULL, resume_dev);
	bus_for_each_dev(&xenbus_backend.bus, NULL, NULL, resume_dev);
}
#endif


/* A flag to determine if xenstored is 'ready' (i.e. has started) */
int xenstored_ready = 0; 


#if 0
int
register_xenstore_notifier(struct notifier_block *nb)
{
	int ret = 0;

	if (xenstored_ready > 0) 
		ret = nb->notifier_call(nb, 0, NULL);
	else 
		notifier_chain_register(&xenstore_chain, nb);

	return ret;
}

void unregister_xenstore_notifier(struct notifier_block *nb)
{
	notifier_chain_unregister(&xenstore_chain, nb);
}
#endif



void
xenbus_probe(void *unused)
{
	KASSERT((xenstored_ready > 0)); 

	/* Enumerate devices in xenstore. */
	xenbus_probe_devices(&xenbus_frontend);
	xenbus_probe_devices(&xenbus_backend);

	/* Watch for changes. */
	fe_watch.node = malloc(strlen("device" + 1), M_DEVBUF, M_NOWAIT);
	strcpy(fe_watch.node, "device");
	fe_watch.xbw_callback = frontend_changed;
	register_xenbus_watch(&fe_watch);
	be_watch.node = malloc(strlen("backend" + 1), M_DEVBUF, M_NOWAIT);
	strcpy(be_watch.node, "backend");
	be_watch.xbw_callback = backend_changed;
	register_xenbus_watch(&be_watch);

	/* Notify others that xenstore is up */
	//notifier_call_chain(&xenstore_chain, 0, NULL);
}

static void
xenbus_probe_init(void *unused)
{
	int err = 0, dom0;

	DPRINTK("");

	SLIST_INIT(&xenbus_device_list);

	/*
	** Domain0 doesn't have a store_evtchn or store_mfn yet.
	*/
	dom0 = (xen_start_info.store_evtchn == 0);
	if (dom0) {
#if defined(DOM0OPS)
		vaddr_t page;
		paddr_t ma;
		evtchn_op_t op = { 0 };
		int ret;

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

		ret = HYPERVISOR_event_channel_op(&op);
		if (ret)
			panic("can't register xenstore event");
		xen_start_info.store_evtchn = op.u.alloc_unbound.port;

		/* And finally publish the above info in /kern/xen */
		xenbus_kernfs_init();
#else /* DOM0OPS */
		return ; /* can't get a working xenstore in this case */
#endif /* DOM0OPS */
	}

	/* Initialize the interface to xenstore. */
	err = xs_init(); 
	if (err) {
		printf("XENBUS: Error initializing xenstore comms: %i\n", err);
		kthread_exit(err);
	}

	if (!dom0) {
		xenstored_ready = 1;
		xenbus_probe(NULL);
	}

	DPRINTK("done");
	config_pending_decr();
	kthread_exit(0);
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
