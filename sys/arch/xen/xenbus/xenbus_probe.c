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

#if 0
#define DPRINTK(fmt, args...) \
    printk("xenbus_probe (%s:%d) " fmt ".\n", __FUNCTION__, __LINE__, ##args)
#else
#define DPRINTK(fmt, args...) ((void)0)
#endif

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/notifier.h>
#include <linux/kthread.h>

#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/hypervisor.h>
#include <asm-xen/xenbus.h>
#include <asm-xen/xen_proc.h>
#include <asm-xen/balloon.h>
#include <asm-xen/evtchn.h>
#include <asm-xen/linux-public/evtchn.h>

#include "xenbus_comms.h"

extern struct semaphore xenwatch_mutex;

#define streq(a, b) (strcmp((a), (b)) == 0)

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

static int xenbus_match(struct device *_dev, struct device_driver *_drv)
{
	struct xenbus_driver *drv = to_xenbus_driver(_drv);

	if (!drv->ids)
		return 0;

	return match_device(drv->ids, to_xenbus_device(_dev)) != NULL;
}

struct xen_bus_type
{
	char *root;
	unsigned int levels;
	int (*get_bus_id)(char bus_id[BUS_ID_SIZE], const char *nodename);
	int (*probe)(const char *type, const char *dir);
	struct bus_type bus;
	struct device dev;
};


/* device/<type>/<id> => <type>-<id> */
static int frontend_bus_id(char bus_id[BUS_ID_SIZE], const char *nodename)
{
	nodename = strchr(nodename, '/');
	if (!nodename || strlen(nodename + 1) >= BUS_ID_SIZE) {
		printk(KERN_WARNING "XENBUS: bad frontend %s\n", nodename);
		return -EINVAL;
	}

	strlcpy(bus_id, nodename + 1, BUS_ID_SIZE);
	if (!strchr(bus_id, '/')) {
		printk(KERN_WARNING "XENBUS: bus_id %s no slash\n", bus_id);
		return -EINVAL;
	}
	*strchr(bus_id, '/') = '-';
	return 0;
}


static int read_otherend_details(struct xenbus_device *xendev,
				 char *id_node, char *path_node)
{
	int err = xenbus_gather(NULL, xendev->nodename,
				id_node, "%i", &xendev->otherend_id,
				path_node, NULL, &xendev->otherend,
				NULL);
	if (err) {
		xenbus_dev_fatal(xendev, err,
				 "reading other end details from %s",
				 xendev->nodename);
		return err;
	}
	if (strlen(xendev->otherend) == 0 ||
	    !xenbus_exists(NULL, xendev->otherend, "")) {
		xenbus_dev_fatal(xendev, -ENOENT, "missing other end from %s",
				 xendev->nodename);
		kfree(xendev->otherend);
		xendev->otherend = NULL;
		return -ENOENT;
	}

	return 0;
}


static int read_backend_details(struct xenbus_device *xendev)
{
	return read_otherend_details(xendev, "backend-id", "backend");
}


static int read_frontend_details(struct xenbus_device *xendev)
{
	return read_otherend_details(xendev, "frontend-id", "frontend");
}


static void free_otherend_details(struct xenbus_device *dev)
{
	kfree(dev->otherend);
	dev->otherend = NULL;
}


static void free_otherend_watch(struct xenbus_device *dev)
{
	if (dev->otherend_watch.node) {
		unregister_xenbus_watch(&dev->otherend_watch);
		kfree(dev->otherend_watch.node);
		dev->otherend_watch.node = NULL;
	}
}


/* Bus type for frontend drivers. */
static int xenbus_probe_frontend(const char *type, const char *name);
static struct xen_bus_type xenbus_frontend = {
	.root = "device",
	.levels = 2, 		/* device/type/<id> */
	.get_bus_id = frontend_bus_id,
	.probe = xenbus_probe_frontend,
	.bus = {
		.name  = "xen",
		.match = xenbus_match,
	},
	.dev = {
		.bus_id = "xen",
	},
};

/* backend/<type>/<fe-uuid>/<id> => <type>-<fe-domid>-<id> */
static int backend_bus_id(char bus_id[BUS_ID_SIZE], const char *nodename)
{
	int domid, err;
	const char *devid, *type, *frontend;
	unsigned int typelen;

	type = strchr(nodename, '/');
	if (!type)
		return -EINVAL;
	type++;
	typelen = strcspn(type, "/");
	if (!typelen || type[typelen] != '/')
		return -EINVAL;

	devid = strrchr(nodename, '/') + 1;

	err = xenbus_gather(NULL, nodename, "frontend-id", "%i", &domid,
			    "frontend", NULL, &frontend,
			    NULL);
	if (err)
		return err;
	if (strlen(frontend) == 0)
		err = -ERANGE;

	if (!err && !xenbus_exists(NULL, frontend, ""))
		err = -ENOENT;

	if (err) {
		kfree(frontend);
		return err;
	}

	if (snprintf(bus_id, BUS_ID_SIZE,
		     "%.*s-%i-%s", typelen, type, domid, devid) >= BUS_ID_SIZE)
		return -ENOSPC;
	return 0;
}

static int xenbus_hotplug_backend(struct device *dev, char **envp,
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
		return -ENODEV;

	xdev = to_xenbus_device(dev);
	if (xdev == NULL)
		return -ENODEV;

	if (dev->driver)
		drv = to_xenbus_driver(dev->driver);

	/* stuff we want to pass to /sbin/hotplug */
	add_hotplug_env_var(envp, num_envp, &i,
			    buffer, buffer_size, &length,
			    "XENBUS_TYPE=%s", xdev->devicetype);

	add_hotplug_env_var(envp, num_envp, &i,
			    buffer, buffer_size, &length,
			    "XENBUS_PATH=%s", xdev->nodename);

	add_hotplug_env_var(envp, num_envp, &i,
			    buffer, buffer_size, &length,
			    "XENBUS_BASE_PATH=%s", xdev->nodename);

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
	kfree(frontend_id);

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
static struct xen_bus_type xenbus_backend = {
	.root = "backend",
	.levels = 3, 		/* backend/type/<frontend>/<id> */
	.get_bus_id = backend_bus_id,
	.probe = xenbus_probe_backend,
	.bus = {
		.name  = "xen-backend",
		.match = xenbus_match,
		.hotplug = xenbus_hotplug_backend,
	},
	.dev = {
		.bus_id = "xen-backend",
	},
};


static void otherend_changed(struct xenbus_watch *watch,
			     const char **vec, unsigned int len)
{
	struct xenbus_device *dev =
		container_of(watch, struct xenbus_device, otherend_watch);
	struct xenbus_driver *drv = to_xenbus_driver(dev->dev.driver);
	XenbusState state;

	/* Protect us against watches firing on old details when the otherend
	   details change, say immediately after a resume. */
	if (!dev->otherend ||
	    strncmp(dev->otherend, vec[XS_WATCH_PATH],
		    strlen(dev->otherend))) {
		DPRINTK("Ignoring watch at %s", vec[XS_WATCH_PATH]);
		return;
	}

	state = xenbus_read_driver_state(dev->otherend);

	DPRINTK("state is %d, %s, %s",
		state, dev->otherend_watch.node, vec[XS_WATCH_PATH]);
	if (drv->otherend_changed)
		drv->otherend_changed(dev, state);
}


static int talk_to_otherend(struct xenbus_device *dev)
{
	struct xenbus_driver *drv = to_xenbus_driver(dev->dev.driver);
	int err;

	free_otherend_watch(dev);
	free_otherend_details(dev);

	err = drv->read_otherend_details(dev);
	if (err)
		return err;

	return xenbus_watch_path2(dev, dev->otherend, "state",
				  &dev->otherend_watch, otherend_changed);
}


static int xenbus_dev_probe(struct device *_dev)
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
		       dev->nodename);
		return err;
	}

	if (!drv->probe) {
		err = -ENODEV;
		goto fail;
	}

	id = match_device(drv->ids, dev);
	if (!id) {
		err = -ENODEV;
		goto fail;
	}

	err = drv->probe(dev, id);
	if (err)
		goto fail;

	return 0;
fail:
	xenbus_dev_error(dev, err, "xenbus_dev_probe on %s", dev->nodename);
	xenbus_switch_state(dev, NULL, XenbusStateClosed);
	return -ENODEV;
	
}

static int xenbus_dev_remove(struct device *_dev)
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

static int xenbus_register_driver_common(struct xenbus_driver *drv,
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

int xenbus_register_frontend(struct xenbus_driver *drv)
{
	drv->read_otherend_details = read_backend_details;

	return xenbus_register_driver_common(drv, &xenbus_frontend);
}
EXPORT_SYMBOL(xenbus_register_frontend);

int xenbus_register_backend(struct xenbus_driver *drv)
{
	drv->read_otherend_details = read_frontend_details;

	return xenbus_register_driver_common(drv, &xenbus_backend);
}
EXPORT_SYMBOL(xenbus_register_backend);

void xenbus_unregister_driver(struct xenbus_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(xenbus_unregister_driver);

struct xb_find_info
{
	struct xenbus_device *dev;
	const char *nodename;
};

static int cmp_dev(struct device *dev, void *data)
{
	struct xenbus_device *xendev = to_xenbus_device(dev);
	struct xb_find_info *info = data;

	if (streq(xendev->nodename, info->nodename)) {
		info->dev = xendev;
		get_device(dev);
		return 1;
	}
	return 0;
}

struct xenbus_device *xenbus_device_find(const char *nodename,
					 struct bus_type *bus)
{
	struct xb_find_info info = { .dev = NULL, .nodename = nodename };

	bus_for_each_dev(bus, NULL, &info, cmp_dev);
	return info.dev;
}

static int cleanup_dev(struct device *dev, void *data)
{
	struct xenbus_device *xendev = to_xenbus_device(dev);
	struct xb_find_info *info = data;
	int len = strlen(info->nodename);

	DPRINTK("%s", info->nodename);

	if (!strncmp(xendev->nodename, info->nodename, len)) {
		info->dev = xendev;
		get_device(dev);
		return 1;
	}
	return 0;
}

static void xenbus_cleanup_devices(const char *path, struct bus_type *bus)
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

static void xenbus_dev_free(struct xenbus_device *xendev)
{
	kfree(xendev);
}

static void xenbus_dev_release(struct device *dev)
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

	p = kmalloc(len + 1, GFP_KERNEL);
	if (!p)
		return NULL;
	va_start(ap, fmt);
	vsprintf(p, fmt, ap);
	va_end(ap);
	return p;
}

static ssize_t xendev_show_nodename(struct device *dev, char *buf)
{
	return sprintf(buf, "%s\n", to_xenbus_device(dev)->nodename);
}
DEVICE_ATTR(nodename, S_IRUSR | S_IRGRP | S_IROTH, xendev_show_nodename, NULL);

static ssize_t xendev_show_devtype(struct device *dev, char *buf)
{
	return sprintf(buf, "%s\n", to_xenbus_device(dev)->devicetype);
}
DEVICE_ATTR(devtype, S_IRUSR | S_IRGRP | S_IROTH, xendev_show_devtype, NULL);


static int xenbus_probe_node(struct xen_bus_type *bus,
			     const char *type,
			     const char *nodename)
{
#define CHECK_FAIL				\
	do {					\
		if (err)			\
			goto fail;		\
	}					\
	while (0)				\


	int err;
	struct xenbus_device *xendev;
	size_t stringlen;
	char *tmpstring;

	XenbusState state = xenbus_read_driver_state(nodename);

	if (state != XenbusStateInitialising) {
		/* Device is not new, so ignore it.  This can happen if a
		   device is going away after switching to Closed.  */
		return 0;
	}

	stringlen = strlen(nodename) + 1 + strlen(type) + 1;
	xendev = kmalloc(sizeof(*xendev) + stringlen, GFP_KERNEL);
	if (!xendev)
		return -ENOMEM;
	memset(xendev, 0, sizeof(*xendev));

	/* Copy the strings into the extra space. */

	tmpstring = (char *)(xendev + 1);
	strcpy(tmpstring, nodename);
	xendev->nodename = tmpstring;

	tmpstring += strlen(tmpstring) + 1;
	strcpy(tmpstring, type);
	xendev->devicetype = tmpstring;

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

	return 0;

#undef CHECK_FAIL

fail:
	xenbus_dev_free(xendev);
	return err;
}

/* device/<typename>/<name> */
static int xenbus_probe_frontend(const char *type, const char *name)
{
	char *nodename;
	int err;

	nodename = kasprintf("%s/%s/%s", xenbus_frontend.root, type, name);
	if (!nodename)
		return -ENOMEM;
	
	DPRINTK("%s", nodename);

	err = xenbus_probe_node(&xenbus_frontend, type, nodename);
	kfree(nodename);
	return err;
}

/* backend/<typename>/<frontend-uuid>/<name> */
static int xenbus_probe_backend_unit(const char *dir,
				     const char *type,
				     const char *name)
{
	char *nodename;
	int err;

	nodename = kasprintf("%s/%s", dir, name);
	if (!nodename)
		return -ENOMEM;

	DPRINTK("%s\n", nodename);

	err = xenbus_probe_node(&xenbus_backend, type, nodename);
	kfree(nodename);
	return err;
}

/* backend/<typename>/<frontend-domid> */
static int xenbus_probe_backend(const char *type, const char *domid)
{
	char *nodename;
	int err = 0;
	char **dir;
	unsigned int i, dir_n = 0;

	DPRINTK("");

	nodename = kasprintf("%s/%s/%s", xenbus_backend.root, type, domid);
	if (!nodename)
		return -ENOMEM;

	dir = xenbus_directory(NULL, nodename, "", &dir_n);
	if (IS_ERR(dir)) {
		kfree(nodename);
		return PTR_ERR(dir);
	}

	for (i = 0; i < dir_n; i++) {
		err = xenbus_probe_backend_unit(nodename, type, dir[i]);
		if (err)
			break;
	}
	kfree(dir);
	kfree(nodename);
	return err;
}

static int xenbus_probe_device_type(struct xen_bus_type *bus, const char *type)
{
	int err = 0;
	char **dir;
	unsigned int dir_n = 0;
	int i;

	dir = xenbus_directory(NULL, bus->root, type, &dir_n);
	if (IS_ERR(dir))
		return PTR_ERR(dir);

	for (i = 0; i < dir_n; i++) {
		err = bus->probe(type, dir[i]);
		if (err)
			break;
	}
	kfree(dir);
	return err;
}

static int xenbus_probe_devices(struct xen_bus_type *bus)
{
	int err = 0;
	char **dir;
	unsigned int i, dir_n;

	dir = xenbus_directory(NULL, bus->root, "", &dir_n);
	if (IS_ERR(dir))
		return PTR_ERR(dir);

	for (i = 0; i < dir_n; i++) {
		err = xenbus_probe_device_type(bus, dir[i]);
		if (err)
			break;
	}
	kfree(dir);
	return err;
}

static unsigned int char_count(const char *str, char c)
{
	unsigned int i, ret = 0;

	for (i = 0; str[i]; i++)
		if (str[i] == c)
			ret++;
	return ret;
}

static int strsep_len(const char *str, char c, unsigned int len)
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

static void dev_changed(const char *node, struct xen_bus_type *bus)
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

	kfree(root);
}

static void frontend_changed(struct xenbus_watch *watch,
			     const char **vec, unsigned int len)
{
	DPRINTK("");

	dev_changed(vec[XS_WATCH_PATH], &xenbus_frontend);
}

static void backend_changed(struct xenbus_watch *watch,
			    const char **vec, unsigned int len)
{
	DPRINTK("");

	dev_changed(vec[XS_WATCH_PATH], &xenbus_backend);
}

/* We watch for devices appearing and vanishing. */
static struct xenbus_watch fe_watch = {
	.node = "device",
	.callback = frontend_changed,
};

static struct xenbus_watch be_watch = {
	.node = "backend",
	.callback = backend_changed,
};

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

static int resume_dev(struct device *dev, void *data)
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

void xenbus_suspend(void)
{
	DPRINTK("");

	bus_for_each_dev(&xenbus_frontend.bus, NULL, NULL, suspend_dev);
	bus_for_each_dev(&xenbus_backend.bus, NULL, NULL, suspend_dev);
	xs_suspend();
}
EXPORT_SYMBOL(xenbus_suspend);

void xenbus_resume(void)
{
	xb_init_comms();
	xs_resume();
	bus_for_each_dev(&xenbus_frontend.bus, NULL, NULL, resume_dev);
	bus_for_each_dev(&xenbus_backend.bus, NULL, NULL, resume_dev);
}
EXPORT_SYMBOL(xenbus_resume);


/* A flag to determine if xenstored is 'ready' (i.e. has started) */
int xenstored_ready = 0; 


int register_xenstore_notifier(struct notifier_block *nb)
{
	int ret = 0;

	if (xenstored_ready > 0) 
		ret = nb->notifier_call(nb, 0, NULL);
	else 
		notifier_chain_register(&xenstore_chain, nb);

	return ret;
}
EXPORT_SYMBOL(register_xenstore_notifier);

void unregister_xenstore_notifier(struct notifier_block *nb)
{
	notifier_chain_unregister(&xenstore_chain, nb);
}
EXPORT_SYMBOL(unregister_xenstore_notifier);



void xenbus_probe(void *unused)
{
	BUG_ON((xenstored_ready <= 0)); 

	/* Enumerate devices in xenstore. */
	xenbus_probe_devices(&xenbus_frontend);
	xenbus_probe_devices(&xenbus_backend);

	/* Watch for changes. */
	register_xenbus_watch(&fe_watch);
	register_xenbus_watch(&be_watch);

	/* Notify others that xenstore is up */
	notifier_call_chain(&xenstore_chain, 0, NULL);
}


static struct proc_dir_entry *xsd_mfn_intf;
static struct proc_dir_entry *xsd_port_intf;


static int xsd_mfn_read(char *page, char **start, off_t off,
                        int count, int *eof, void *data)
{
	int len; 
	len  = sprintf(page, "%ld", xen_start_info->store_mfn); 
	*eof = 1; 
	return len; 
}

static int xsd_port_read(char *page, char **start, off_t off,
			 int count, int *eof, void *data)
{
	int len; 

	len  = sprintf(page, "%d", xen_start_info->store_evtchn); 
	*eof = 1; 
	return len; 
}


static int __init xenbus_probe_init(void)
{
	int err = 0, dom0;

	DPRINTK("");

	if (xen_init() < 0) {
		DPRINTK("failed");
		return -ENODEV;
	}

	/* Register ourselves with the kernel bus & device subsystems */
	bus_register(&xenbus_frontend.bus);
	bus_register(&xenbus_backend.bus);
	device_register(&xenbus_frontend.dev);
	device_register(&xenbus_backend.dev);

	/*
	** Domain0 doesn't have a store_evtchn or store_mfn yet.
	*/
	dom0 = (xen_start_info->store_evtchn == 0);

	if (dom0) {

		unsigned long page;
		evtchn_op_t op = { 0 };
		int ret;


		/* Allocate page. */
		page = get_zeroed_page(GFP_KERNEL);
		if (!page) 
			return -ENOMEM; 

		/* We don't refcnt properly, so set reserved on page.
		 * (this allocation is permanent) */
		SetPageReserved(virt_to_page(page));

		xen_start_info->store_mfn =
			pfn_to_mfn(virt_to_phys((void *)page) >>
				   PAGE_SHIFT);
		
		/* Next allocate a local port which xenstored can bind to */
		op.cmd = EVTCHNOP_alloc_unbound;
		op.u.alloc_unbound.dom        = DOMID_SELF;
		op.u.alloc_unbound.remote_dom = 0; 

		ret = HYPERVISOR_event_channel_op(&op);
		BUG_ON(ret); 
		xen_start_info->store_evtchn = op.u.alloc_unbound.port;

		/* And finally publish the above info in /proc/xen */
		if((xsd_mfn_intf = create_xen_proc_entry("xsd_mfn", 0400)))
			xsd_mfn_intf->read_proc = xsd_mfn_read; 
		if((xsd_port_intf = create_xen_proc_entry("xsd_port", 0400)))
			xsd_port_intf->read_proc = xsd_port_read;
	}

	/* Initialize the interface to xenstore. */
	err = xs_init(); 
	if (err) {
		printk(KERN_WARNING
		       "XENBUS: Error initializing xenstore comms: %i\n", err);
		return err; 
	}

	if (!dom0) {
		xenstored_ready = 1;
		xenbus_probe(NULL);
	}

	return 0;
}

postcore_initcall(xenbus_probe_init);

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
