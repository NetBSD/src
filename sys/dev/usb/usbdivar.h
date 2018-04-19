/*	$NetBSD: usbdivar.h,v 1.115 2018/04/19 21:50:09 christos Exp $	*/

/*
 * Copyright (c) 1998, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology and Matthew R. Green (mrg@eterna.com.au).
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

/*
 * Discussion about locking in the USB code:
 *
 * The host controller presents one lock at IPL_SOFTUSB (aka IPL_SOFTNET).
 *
 * List of hardware interface methods, and whether the lock is held
 * when each is called by this module:
 *
 *	BUS METHOD		LOCK	NOTES
 *	----------------------- -------	-------------------------
 *	ubm_open		-	might want to take lock?
 *	ubm_softint		x
 *	ubm_dopoll		-	might want to take lock?
 *	ubm_allocx		-
 *	ubm_freex		-
 *	ubm_getlock 		-	Called at attach time
 *	ubm_newdev		-	Will take lock
	ubm_rhctrl
 *
 *	PIPE METHOD		LOCK	NOTES
 *	----------------------- -------	-------------------------
 *	upm_transfer		-
 *	upm_start		-	might want to take lock?
 *	upm_abort		x
 *	upm_close		x
 *	upm_cleartoggle		-
 *	upm_done		x
 *
 * The above semantics are likely to change.  Little performance
 * evaluation has been done on this code and the locking strategy.
 *
 * USB functions known to expect the lock taken include (this list is
 * probably not exhaustive):
 *    usb_transfer_complete()
 *    usb_insert_transfer()
 *    usb_start_next()
 *
 */

#include <sys/callout.h>
#include <sys/mutex.h>
#include <sys/bus.h>

/* From usb_mem.h */
struct usb_dma_block;
typedef struct {
	struct usb_dma_block *udma_block;
	u_int udma_offs;
} usb_dma_t;

struct usbd_xfer;
struct usbd_pipe;
struct usbd_port;

struct usbd_endpoint {
	usb_endpoint_descriptor_t *ue_edesc;
	int			ue_refcnt;
	int			ue_toggle;
};

struct usbd_bus_methods {
	usbd_status	      (*ubm_open)(struct usbd_pipe *);
	void		      (*ubm_softint)(void *);
	void		      (*ubm_dopoll)(struct usbd_bus *);
	struct usbd_xfer     *(*ubm_allocx)(struct usbd_bus *, unsigned int);
	void		      (*ubm_freex)(struct usbd_bus *, struct usbd_xfer *);
	void		      (*ubm_getlock)(struct usbd_bus *, kmutex_t **);
	usbd_status	      (*ubm_newdev)(device_t, struct usbd_bus *, int,
					    int, int, struct usbd_port *);

	int			(*ubm_rhctrl)(struct usbd_bus *,
				    usb_device_request_t *, void *, int);
};

struct usbd_pipe_methods {
	int		      (*upm_init)(struct usbd_xfer *);
	void		      (*upm_fini)(struct usbd_xfer *);
	usbd_status	      (*upm_transfer)(struct usbd_xfer *);
	usbd_status	      (*upm_start)(struct usbd_xfer *);
	void		      (*upm_abort)(struct usbd_xfer *);
	void		      (*upm_close)(struct usbd_pipe *);
	void		      (*upm_cleartoggle)(struct usbd_pipe *);
	void		      (*upm_done)(struct usbd_xfer *);
};

struct usbd_tt {
	struct usbd_hub	       *utt_hub;
};

struct usbd_port {
	usb_port_status_t	up_status;
	uint16_t		up_power;	/* mA of current on port */
	uint8_t			up_portno;
	uint8_t			up_restartcnt;
#define USBD_RESTART_MAX 5
	uint8_t			up_reattach;
	struct usbd_device     *up_dev;		/* Connected device */
	struct usbd_device     *up_parent;	/* The ports hub */
	struct usbd_tt	       *up_tt;	/* Transaction translator (if any) */
};

struct usbd_hub {
	usbd_status	      (*uh_explore)(struct usbd_device *hub);
	void		       *uh_hubsoftc;
	usb_hub_descriptor_t	uh_hubdesc;
	struct usbd_port        uh_ports[1];
};

/*****/
/* 0, root, and 1->127 */
#define USB_ROOTHUB_INDEX	1
#define USB_TOTAL_DEVICES	(USB_MAX_DEVICES + 1)

struct usbd_bus {
	/* Filled by HC driver */
	void			*ub_hcpriv;
	int			ub_revision;	/* USB revision */
#define USBREV_UNKNOWN	0
#define USBREV_PRE_1_0	1
#define USBREV_1_0	2
#define USBREV_1_1	3
#define USBREV_2_0	4
#define USBREV_3_0	5
#define USBREV_STR { "unknown", "pre 1.0", "1.0", "1.1", "2.0", "3.0" }

	const struct usbd_bus_methods
			       *ub_methods;
	uint32_t		ub_pipesize;	/* size of a pipe struct */
	bool			ub_usedma;	/* Does this HC support DMA */
	int			ub_dmaflags;
	bus_dma_tag_t		ub_dmatag;	/* DMA tag */

	/* Filled by usb driver */
	kmutex_t	       *ub_lock;
	struct usbd_device     *ub_roothub;
	uint8_t			ub_rhaddr;	/* roothub address */
	uint8_t			ub_rhconf;	/* roothub configuration */
	struct usbd_device     *ub_devices[USB_TOTAL_DEVICES];
	kcondvar_t              ub_needsexplore_cv;
	char			ub_needsexplore;/* a hub a signalled a change */
	char			ub_usepolling;
	device_t		ub_usbctl;
	struct usb_device_stats	ub_stats;

	void		       *ub_soft; /* soft interrupt cookie */
};

struct usbd_device {
	struct usbd_bus	       *ud_bus;		/* our controller */
	struct usbd_pipe       *ud_pipe0;	/* pipe 0 */
	uint8_t			ud_addr;	/* device addess */
	uint8_t			ud_config;	/* current configuration # */
	uint8_t			ud_depth;	/* distance from root hub */
	uint8_t			ud_speed;	/* low/full/high speed */
	uint8_t			ud_selfpowered;	/* flag for self powered */
	uint16_t		ud_power;	/* mA the device uses */
	int16_t			ud_langid;	/* language for strings */
#define USBD_NOLANG (-1)
	usb_event_cookie_t	ud_cookie;	/* unique connection id */
	struct usbd_port       *ud_powersrc;	/* upstream hub port, or 0 */
	struct usbd_device     *ud_myhub;	/* upstream hub */
	struct usbd_port       *ud_myhsport;	/* closest high speed port */
	struct usbd_endpoint	ud_ep0;		/* for pipe 0 */
	usb_endpoint_descriptor_t
				ud_ep0desc;	/* for pipe 0 */
	struct usbd_interface  *ud_ifaces;	/* array of all interfaces */
	usb_device_descriptor_t ud_ddesc;	/* device descriptor */
	usb_config_descriptor_t *ud_cdesc;	/* full config descr */
	usb_bos_descriptor_t	*ud_bdesc;	/* full BOS descr */
	const struct usbd_quirks
			       *ud_quirks;	/* device quirks, always set */
	struct usbd_hub	       *ud_hub;		/* only if this is a hub */
	int			ud_subdevlen;	/* array length of following */
	device_t	       *ud_subdevs;	/* sub-devices */
	int			ud_nifaces_claimed; /* number of ifaces in use */
	void		       *ud_hcpriv;

	char		       *ud_serial;	/* serial number, can be NULL */
	char		       *ud_vendor;	/* vendor string, can be NULL */
	char		       *ud_product;	/* product string can be NULL */
};

struct usbd_interface {
	struct usbd_device     *ui_dev;
	usb_interface_descriptor_t
			       *ui_idesc;
	int			ui_index;
	int			ui_altindex;
	struct usbd_endpoint   *ui_endpoints;
	void		       *ui_priv;
	LIST_HEAD(, usbd_pipe)	ui_pipes;
};

struct usbd_pipe {
	struct usbd_interface  *up_iface;
	struct usbd_device     *up_dev;
	struct usbd_endpoint   *up_endpoint;
	char			up_running;
	char			up_aborting;
	bool			up_serialise;
	SIMPLEQ_HEAD(, usbd_xfer)
			        up_queue;
	LIST_ENTRY(usbd_pipe)	up_next;
	struct usb_task		up_async_task;

	struct usbd_xfer       *up_intrxfer; /* used for repeating requests */
	char			up_repeat;
	int			up_interval;
	uint8_t			up_flags;

	/* Filled by HC driver. */
	const struct usbd_pipe_methods
			       *up_methods;
};

struct usbd_xfer {
	struct usbd_pipe       *ux_pipe;
	void		       *ux_priv;
	void		       *ux_buffer;
	kcondvar_t		ux_cv;
	uint32_t		ux_length;
	uint32_t		ux_actlen;
	uint16_t		ux_flags;
	uint32_t		ux_timeout;
	usbd_status		ux_status;
	usbd_callback		ux_callback;
	volatile uint8_t	ux_done;
	uint8_t			ux_state;	/* used for DIAGNOSTIC */
#define XFER_FREE 0x46
#define XFER_BUSY 0x55
#define XFER_ONQU 0x9e

	/* For control pipe */
	usb_device_request_t	ux_request;

	/* For isoc */
	uint16_t	       *ux_frlengths;
	int			ux_nframes;

	const struct usbd_pipe_methods *ux_methods;

	/* For memory allocation and softc */
	struct usbd_bus        *ux_bus;
	usb_dma_t		ux_dmabuf;
	void		       *ux_buf;
	uint32_t		ux_bufsize;

	uint8_t			ux_rqflags;
#define URQ_REQUEST	0x01

	SIMPLEQ_ENTRY(usbd_xfer)
				ux_next;

	void		       *ux_hcpriv;	/* private use by the HC driver */
	uint8_t			ux_hcflags;	/* private use by the HC driver */
#define UXFER_ABORTING	0x01	/* xfer is aborting. */
#define UXFER_ABORTWAIT	0x02	/* abort completion is being awaited. */
	kcondvar_t		ux_hccv;	/* private use by the HC driver */

	struct callout		ux_callout;
};

void usbd_init(void);
void usbd_finish(void);

#if defined(USB_DEBUG)
void usbd_dump_iface(struct usbd_interface *);
void usbd_dump_device(struct usbd_device *);
void usbd_dump_endpoint(struct usbd_endpoint *);
void usbd_dump_queue(struct usbd_pipe *);
void usbd_dump_pipe(struct usbd_pipe *);
#endif

/* Routines from usb_subr.c */
int		usbctlprint(void *, const char *);
void		usbd_get_device_strings(struct usbd_device *);
void		usb_delay_ms_locked(struct usbd_bus *, u_int, kmutex_t *);
void		usb_delay_ms(struct usbd_bus *, u_int);
void		usbd_delay_ms_locked(struct usbd_device *, u_int, kmutex_t *);
void		usbd_delay_ms(struct usbd_device *, u_int);
usbd_status	usbd_reset_port(struct usbd_device *, int, usb_port_status_t *);
usbd_status	usbd_setup_pipe(struct usbd_device *,
				struct usbd_interface *,
				struct usbd_endpoint *, int,
				struct usbd_pipe **);
usbd_status	usbd_setup_pipe_flags(struct usbd_device *,
				      struct usbd_interface *,
				      struct usbd_endpoint *, int,
				      struct usbd_pipe **,
				      uint8_t);
usbd_status	usbd_new_device(device_t, struct usbd_bus *, int, int, int,
				struct usbd_port *);
usbd_status	usbd_reattach_device(device_t, struct usbd_device *,
				     int, const int *);

void		usbd_remove_device(struct usbd_device *, struct usbd_port *);
int		usbd_printBCD(char *, size_t, int);
usbd_status	usbd_fill_iface_data(struct usbd_device *, int, int);
void		usb_free_device(struct usbd_device *);

usbd_status	usb_insert_transfer(struct usbd_xfer *);
void		usb_transfer_complete(struct usbd_xfer *);
int		usb_disconnect_port(struct usbd_port *, device_t, int);

void		usbd_kill_pipe(struct usbd_pipe *);
usbd_status	usbd_attach_roothub(device_t, struct usbd_device *);
usbd_status	usbd_probe_and_attach(device_t, struct usbd_device *, int, int);
usbd_status	usbd_get_initial_ddesc(struct usbd_device *,
				       usb_device_descriptor_t *);

/* Routines from usb.c */
void		usb_needs_explore(struct usbd_device *);
void		usb_needs_reattach(struct usbd_device *);
void		usb_schedsoftintr(struct usbd_bus *);

static __inline int
usbd_xfer_isread(struct usbd_xfer *xfer)
{
	if (xfer->ux_rqflags & URQ_REQUEST)
		return xfer->ux_request.bmRequestType & UT_READ;

	return xfer->ux_pipe->up_endpoint->ue_edesc->bEndpointAddress &
	   UE_DIR_IN;
}

static __inline size_t
usb_addr2dindex(int addr)
{

	return USB_ROOTHUB_INDEX + addr;
}

/*
 * These macros reflect the current locking scheme.  They might change.
 */

#define usbd_lock_pipe(p)	mutex_enter((p)->up_dev->ud_bus->ub_lock)
#define usbd_unlock_pipe(p)	mutex_exit((p)->up_dev->ud_bus->ub_lock)
