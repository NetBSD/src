/*	$NetBSD: usbdi.h,v 1.90.2.2 2018/08/08 10:17:11 martin Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/usbdi.h,v 1.18 1999/11/17 22:33:49 n_hibma Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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

#ifndef _USBDI_H_
#define _USBDI_H_

struct usbd_bus;
struct usbd_device;
struct usbd_interface;
struct usbd_pipe;
struct usbd_xfer;

typedef enum {		/* keep in sync with usbd_error_strs */
	USBD_NORMAL_COMPLETION = 0, /* must be 0 */
	USBD_IN_PROGRESS,	/* 1 */
	/* errors */
	USBD_PENDING_REQUESTS,	/* 2 */
	USBD_NOT_STARTED,	/* 3 */
	USBD_INVAL,		/* 4 */
	USBD_NOMEM,		/* 5 */
	USBD_CANCELLED,		/* 6 */
	USBD_BAD_ADDRESS,	/* 7 */
	USBD_IN_USE,		/* 8 */
	USBD_NO_ADDR,		/* 9 */
	USBD_SET_ADDR_FAILED,	/* 10 */
	USBD_NO_POWER,		/* 11 */
	USBD_TOO_DEEP,		/* 12 */
	USBD_IOERROR,		/* 13 */
	USBD_NOT_CONFIGURED,	/* 14 */
	USBD_TIMEOUT,		/* 15 */
	USBD_SHORT_XFER,	/* 16 */
	USBD_STALLED,		/* 17 */
	USBD_INTERRUPTED,	/* 18 */

	USBD_ERROR_MAX		/* must be last */
} usbd_status;

typedef void (*usbd_callback)(struct usbd_xfer *, void *, usbd_status);

/* Use default (specified by ep. desc.) interval on interrupt pipe */
#define USBD_DEFAULT_INTERVAL	(-1)

/* Open flags */
#define USBD_EXCLUSIVE_USE	0x01
#define USBD_MPSAFE		0x80

/* Request flags */
#define USBD_SYNCHRONOUS	0x02	/* wait for completion */
/* in usb.h #define USBD_SHORT_XFER_OK	0x04*/	/* allow short reads */
#define USBD_FORCE_SHORT_XFER	0x08	/* force last short packet on write */
#define USBD_SYNCHRONOUS_SIG	0x10	/* if waiting for completion,
					 * also take signals */

#define USBD_NO_TIMEOUT 0
#define USBD_DEFAULT_TIMEOUT 5000 /* ms = 5 s */
#define	USBD_CONFIG_TIMEOUT  (3*USBD_DEFAULT_TIMEOUT)

#define DEVINFOSIZE 1024

usbd_status usbd_open_pipe_intr(struct usbd_interface *, uint8_t, uint8_t,
    struct usbd_pipe **, void *, void *, uint32_t, usbd_callback, int);
usbd_status usbd_open_pipe(struct usbd_interface *, uint8_t, uint8_t,
     struct usbd_pipe **);
usbd_status usbd_close_pipe(struct usbd_pipe *);

usbd_status usbd_transfer(struct usbd_xfer *);

void *usbd_get_buffer(struct usbd_xfer *);
struct usbd_pipe *usbd_get_pipe0(struct usbd_device *);

int usbd_create_xfer(struct usbd_pipe *, size_t, unsigned int, unsigned int,
    struct usbd_xfer **);
void usbd_destroy_xfer(struct usbd_xfer *);

void usbd_setup_xfer(struct usbd_xfer *, void *, void *,
    uint32_t, uint16_t, uint32_t, usbd_callback);

void usbd_setup_default_xfer(struct usbd_xfer *, struct usbd_device *,
    void *, uint32_t, usb_device_request_t *, void *,
    uint32_t, uint16_t, usbd_callback);

void usbd_setup_isoc_xfer(struct usbd_xfer *, void *, uint16_t *,
    uint32_t, uint16_t, usbd_callback);

void usbd_get_xfer_status(struct usbd_xfer *, void **,
    void **, uint32_t *, usbd_status *);

usb_endpoint_descriptor_t *usbd_interface2endpoint_descriptor
    (struct usbd_interface *, uint8_t);

usbd_status usbd_abort_pipe(struct usbd_pipe *);
usbd_status usbd_abort_default_pipe(struct usbd_device *);

usbd_status usbd_clear_endpoint_stall(struct usbd_pipe *);
void usbd_clear_endpoint_stall_async(struct usbd_pipe *);

void usbd_clear_endpoint_toggle(struct usbd_pipe *);
usbd_status usbd_endpoint_count(struct usbd_interface *, uint8_t *);

usbd_status usbd_interface_count(struct usbd_device *, uint8_t *);

void usbd_interface2device_handle(struct usbd_interface *, struct usbd_device **);
usbd_status usbd_device2interface_handle(struct usbd_device *,
    uint8_t, struct usbd_interface **);

struct usbd_device *usbd_pipe2device_handle(struct usbd_pipe *);

usbd_status usbd_sync_transfer(struct usbd_xfer *);
usbd_status usbd_sync_transfer_sig(struct usbd_xfer *);

usbd_status usbd_do_request(struct usbd_device *, usb_device_request_t *, void *);
usbd_status usbd_do_request_flags(struct usbd_device *, usb_device_request_t *,
    void *, uint16_t, int *, uint32_t);

usb_interface_descriptor_t *
    usbd_get_interface_descriptor(struct usbd_interface *);
usb_endpoint_descriptor_t *
    usbd_get_endpoint_descriptor(struct usbd_interface *, uint8_t);

usb_config_descriptor_t *usbd_get_config_descriptor(struct usbd_device *);
usb_device_descriptor_t *usbd_get_device_descriptor(struct usbd_device *);

usbd_status usbd_set_interface(struct usbd_interface *, int);
usbd_status usbd_get_interface(struct usbd_interface *, uint8_t *);

int usbd_get_no_alts(usb_config_descriptor_t *, int);

void usbd_fill_deviceinfo(struct usbd_device *, struct usb_device_info *, int);
#ifdef COMPAT_30
void usbd_fill_deviceinfo_old(struct usbd_device *, struct usb_device_info_old *,
    int);
#endif
int usbd_get_interface_altindex(struct usbd_interface *);

usb_interface_descriptor_t *usbd_find_idesc(usb_config_descriptor_t *,
    int, int);
usb_endpoint_descriptor_t *usbd_find_edesc(usb_config_descriptor_t *,
    int, int, int);

void usbd_dopoll(struct usbd_interface *);
void usbd_set_polling(struct usbd_device *, int);

const char *usbd_errstr(usbd_status);

void usbd_add_dev_event(int, struct usbd_device *);
void usbd_add_drv_event(int, struct usbd_device *, device_t);

char *usbd_devinfo_alloc(struct usbd_device *, int);
void usbd_devinfo_free(char *);

const struct usbd_quirks *usbd_get_quirks(struct usbd_device *);

usbd_status usbd_reload_device_desc(struct usbd_device *);

int usbd_ratecheck(struct timeval *);

usbd_status usbd_get_string(struct usbd_device *, int, char *);
usbd_status usbd_get_string0(struct usbd_device *, int, char *, int);

/* An iterator for descriptors. */
typedef struct {
	const uByte *cur;
	const uByte *end;
} usbd_desc_iter_t;
void usb_desc_iter_init(struct usbd_device *, usbd_desc_iter_t *);
const usb_descriptor_t *usb_desc_iter_next(usbd_desc_iter_t *);

/* Used to clear endpoint stalls from the softint */
void usbd_clear_endpoint_stall_task(void *);

/*
 * The usb_task structs form a queue of things to run in the USB event
 * thread.  Normally this is just device discovery when a connect/disconnect
 * has been detected.  But it may also be used by drivers that need to
 * perform (short) tasks that must have a process context.
 */
struct usb_task {
	TAILQ_ENTRY(usb_task) next;
	void (*fun)(void *);
	void *arg;
	volatile unsigned queue;
	int flags;
};
#define	USB_TASKQ_HC		0
#define	USB_TASKQ_DRIVER	1
#define	USB_NUM_TASKQS		2
#define	USB_TASKQ_NAMES		{"usbtask-hc", "usbtask-dr"}
#define	USB_TASKQ_MPSAFE	0x80

void usb_add_task(struct usbd_device *, struct usb_task *, int);
void usb_rem_task(struct usbd_device *, struct usb_task *);
bool usb_rem_task_wait(struct usbd_device *, struct usb_task *, int,
    kmutex_t *);
#define usb_init_task(t, f, a, fl) ((t)->fun = (f), (t)->arg = (a), (t)->queue = USB_NUM_TASKQS, (t)->flags = (fl))

struct usb_devno {
	uint16_t ud_vendor;
	uint16_t ud_product;
};
const struct usb_devno *usb_match_device(const struct usb_devno *,
	u_int, u_int, uint16_t, uint16_t);
#define usb_lookup(tbl, vendor, product) \
	usb_match_device((const struct usb_devno *)(tbl), sizeof(tbl) / sizeof((tbl)[0]), sizeof((tbl)[0]), (vendor), (product))
#define	USB_PRODUCT_ANY		0xffff

/* NetBSD attachment information */

/* Attach data */
struct usb_attach_arg {
	int			uaa_port;
	int			uaa_vendor;
	int			uaa_product;
	int			uaa_release;
	struct usbd_device *	uaa_device;	/* current device */
	int			uaa_class;
	int			uaa_subclass;
	int			uaa_proto;
	int			uaa_usegeneric;
};

struct usbif_attach_arg {
	int			uiaa_port;
	int			uiaa_configno;
	int			uiaa_ifaceno;
	int			uiaa_vendor;
	int			uiaa_product;
	int			uiaa_release;
	struct usbd_device *	uiaa_device;	/* current device */

	struct usbd_interface *	uiaa_iface;	/* current interface */
	int			uiaa_class;
	int			uiaa_subclass;
	int			uiaa_proto;

	/* XXX need accounting for interfaces not matched to */

	struct usbd_interface **uiaa_ifaces;	/* all interfaces */
	int			uiaa_nifaces;	/* number of interfaces */
};

/* Match codes. */
#define UMATCH_HIGHEST					15
/* First five codes is for a whole device. */
#define UMATCH_VENDOR_PRODUCT_REV			14
#define UMATCH_VENDOR_PRODUCT				13
#define UMATCH_VENDOR_DEVCLASS_DEVPROTO			12
#define UMATCH_DEVCLASS_DEVSUBCLASS_DEVPROTO		11
#define UMATCH_DEVCLASS_DEVSUBCLASS			10
/* Next six codes are for interfaces. */
#define UMATCH_VENDOR_PRODUCT_REV_CONF_IFACE		 9
#define UMATCH_VENDOR_PRODUCT_CONF_IFACE		 8
#define UMATCH_VENDOR_IFACESUBCLASS_IFACEPROTO		 7
#define UMATCH_VENDOR_IFACESUBCLASS			 6
#define UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO	 5
#define UMATCH_IFACECLASS_IFACESUBCLASS			 4
#define UMATCH_IFACECLASS				 3
#define UMATCH_IFACECLASS_GENERIC			 2
/* Generic driver */
#define UMATCH_GENERIC					 1
/* No match */
#define UMATCH_NONE					 0


/*
 * IPL_USB is defined as IPL_VM for drivers that have not been made MP safe.
 * IPL_VM (currently) takes the kernel lock.
 *
 * Eventually, IPL_USB can/should be changed
 */
#define IPL_USB IPL_VM
#define splhardusb splvm

#define SOFTINT_USB SOFTINT_SERIAL
#define IPL_SOFTUSB IPL_SOFTSERIAL
#define splusb splsoftserial

#endif /* _USBDI_H_ */
