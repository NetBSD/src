/* $NetBSD: xenbus.h,v 1.4.4.2 2006/04/22 11:38:11 simonb Exp $ */
/******************************************************************************
 * xenbus.h
 *
 * Talks to Xen Store to figure out what devices we have.
 *
 * Copyright (C) 2005 Rusty Russell, IBM Corporation
 * Copyright (C) 2005 XenSource Ltd.
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

#ifndef _ASM_XEN_XENBUS_H
#define _ASM_XEN_XENBUS_H

#include <sys/device.h>
#include <sys/queue.h>
#include <machine/xen3-public/io/xenbus.h>
#include <machine/xen3-public/io/xs_wire.h>

/* xenbus to hypervisor attach */
struct xenbus_attach_args {
	const char 		*xa_device;
	int			xa_handle;
};

/* devices to xenbus attach */
struct xenbusdev_attach_args {
	const char		*xa_type;
	int 			xa_id;
	struct xenbus_device	*xa_xbusd;
};

/* Register callback to watch this node. */
struct xenbus_watch {
	SLIST_ENTRY(xenbus_watch) watch_next;

	/* Path being watched. */
	char *node;

	/* Callback (executed in a process context with no locks held). */
	void (*xbw_callback)(struct xenbus_watch *,
			 const char **vec, unsigned int len);
	struct xenbus_device *xbw_dev;
};


/*
 * A xenbus device. Note that the malloced memory will be larger than
 * sizeof(xenbus_device) to have the storage for xbusd_path, so xbusd_path
 * has to be the last entry.
 */
struct xenbus_device {
	SLIST_ENTRY(xenbus_device) xbusd_entries;
	char *xbusd_otherend; /* the otherend path */
	int xbusd_otherend_id; /* the otherend's id */
	/* callback for otherend change */
	void (*xbusd_otherend_changed)(struct device *, XenbusState);
	struct device *xbusd_dev;
	int xbusd_has_error;
	/* for xenbus internal use */
	struct xenbus_watch xbusd_otherend_watch;
	const char xbusd_path[1]; /* our path */
};

struct xenbus_device_id
{
	/* .../device/<device_type>/<identifier> */
	char devicetype[32]; 	/* General class of device. */
};

struct xenbus_transaction;

int xenbus_directory(struct xenbus_transaction *t,
			const char *dir, const char *node, unsigned int *num,
			char ***);
int xenbus_read(struct xenbus_transaction *t,
		  const char *dir, const char *node, unsigned int *len,
		  char **);
int xenbus_read_ul(struct xenbus_transaction *,
		  const char *, const char *, unsigned long *);
int xenbus_write(struct xenbus_transaction *t,
		 const char *dir, const char *node, const char *string);
int xenbus_mkdir(struct xenbus_transaction *t,
		 const char *dir, const char *node);
int xenbus_exists(struct xenbus_transaction *t,
		  const char *dir, const char *node);
int xenbus_rm(struct xenbus_transaction *t, const char *dir, const char *node);
struct xenbus_transaction *xenbus_transaction_start(void);
int xenbus_transaction_end(struct xenbus_transaction *t, int abort);

/* Single read and scanf: returns -errno or num scanned if > 0. */
int xenbus_scanf(struct xenbus_transaction *t,
		 const char *dir, const char *node, const char *fmt, ...)
	__attribute__((format(scanf, 4, 5)));

/* Single printf and write: returns -errno or 0. */
int xenbus_printf(struct xenbus_transaction *t,
		  const char *dir, const char *node, const char *fmt, ...)
	__attribute__((format(printf, 4, 5)));

/* Generic read function: NULL-terminated triples of name,
 * sprintf-style type string, and pointer. Returns 0 or errno.*/
int xenbus_gather(struct xenbus_transaction *t, const char *dir, ...);

/* notifer routines for when the xenstore comes up */
// XXX int register_xenstore_notifier(struct notifier_block *nb);
// XXX void unregister_xenstore_notifier(struct notifier_block *nb);

int register_xenbus_watch(struct xenbus_watch *watch);
void unregister_xenbus_watch(struct xenbus_watch *watch);
void xs_suspend(void);
void xs_resume(void);

/* Used by xenbus_dev to borrow kernel's store connection. */
int xenbus_dev_request_and_reply(struct xsd_sockmsg *msg, void **);

/* Called from xen core code. */
void xenbus_suspend(void);
void xenbus_resume(void);

void xenbus_probe(void *);

int xenbus_free_device(struct xenbus_device *);

#define XENBUS_IS_ERR_READ(str) ({			\
	if (!IS_ERR(str) && strlen(str) == 0) {		\
		kfree(str);				\
		str = ERR_PTR(-ERANGE);			\
	}						\
	IS_ERR(str);					\
})

#define XENBUS_EXIST_ERR(err) ((err) == -ENOENT || (err) == -ERANGE)


/**
 * Register a watch on the given path, using the given xenbus_watch structure
 * for storage, and the given callback function as the callback.  Return 0 on
 * success, or -errno on error.  On success, the given path will be saved as
 * watch->node, and remains the caller's to free.  On error, watch->node will
 * be NULL, the device will switch to XenbusStateClosing, and the error will
 * be saved in the store.
 */
int xenbus_watch_path(struct xenbus_device *dev, char *path,
		      struct xenbus_watch *watch, 
		      void (*callback)(struct xenbus_watch *,
				       const char **, unsigned int));


/**
 * Register a watch on the given path/path2, using the given xenbus_watch
 * structure for storage, and the given callback function as the callback.
 * Return 0 on success, or -errno on error.  On success, the watched path
 * (path/path2) will be saved as watch->node, and becomes the caller's to
 * kfree().  On error, watch->node will be NULL, so the caller has nothing to
 * free, the device will switch to XenbusStateClosing, and the error will be
 * saved in the store.
 */
int xenbus_watch_path2(struct xenbus_device *dev, const char *path,
		       const char *path2, struct xenbus_watch *watch, 
		       void (*callback)(struct xenbus_watch *,
					const char **, unsigned int));


/**
 * Advertise in the store a change of the given driver to the given new_state.
 * Perform the change inside the given transaction xbt.  xbt may be NULL, in
 * which case this is performed inside its own transaction.  Return 0 on
 * success, or -errno on error.  On error, the device will switch to
 * XenbusStateClosing, and the error will be saved in the store.
 */
int xenbus_switch_state(struct xenbus_device *dev,
			struct xenbus_transaction *xbt,
			XenbusState new_state);


/**
 * Grant access to the given ring_mfn to the peer of the given device.  Return
 * 0 on success, or -errno on error.  On error, the device will switch to
 * XenbusStateClosing, and the error will be saved in the store.
 */
int xenbus_grant_ring(struct xenbus_device *, paddr_t, grant_ref_t *);


/**
 * Allocate an event channel for the given xenbus_device, assigning the newly
 * created local port to *port.  Return 0 on success, or -errno on error.  On
 * error, the device will switch to XenbusStateClosing, and the error will be
 * saved in the store.
 */
int xenbus_alloc_evtchn(struct xenbus_device *dev, int *port);


/**
 * Return the state of the driver rooted at the given store path, or
 * XenbusStateClosed if no state can be read.
 */
XenbusState xenbus_read_driver_state(const char *path);


/***
 * Report the given negative errno into the store, along with the given
 * formatted message.
 */
void xenbus_dev_error(struct xenbus_device *dev, int err, const char *fmt,
		      ...);


/***
 * Equivalent to xenbus_dev_error(dev, err, fmt, args), followed by
 * xenbus_switch_state(dev, NULL, XenbusStateClosing) to schedule an orderly
 * closedown of this driver and its peer.
 */
void xenbus_dev_fatal(struct xenbus_device *dev, int err, const char *fmt,
		      ...);


#endif /* _ASM_XEN_XENBUS_H */

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
