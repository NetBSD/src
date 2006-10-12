/*	$NetBSD: umidi.c,v 1.27 2006/10/12 01:32:00 christos Exp $	*/
/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takuya SHIOZAKI (tshiozak@NetBSD.org) and (full-size transfers, extended
 * hw_if) Chapman Flack (chap@NetBSD.org).
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
 *	  This product includes software developed by the NetBSD
 *	  Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: umidi.c,v 1.27 2006/10/12 01:32:00 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/poll.h>
#include <sys/lock.h>

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
#include <machine/intr.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>

#include <dev/usb/usbdevs.h>
#include <dev/usb/uaudioreg.h>
#include <dev/usb/umidireg.h>
#include <dev/usb/umidivar.h>
#include <dev/usb/umidi_quirks.h>

#include <dev/midi_if.h>

#ifdef UMIDI_DEBUG
#define DPRINTF(x)	if (umididebug) printf x
#define DPRINTFN(n,x)	if (umididebug >= (n)) printf x
#include <sys/time.h>
static struct timeval umidi_tv;
int	umididebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif


static int umidi_open(void *, int,
		      void (*)(void *, int), void (*)(void *), void *);
static void umidi_close(void *);
static int umidi_channelmsg(void *, int, int, u_char *, int);
static int umidi_commonmsg(void *, int, u_char *, int);
static int umidi_sysex(void *, u_char *, int);
static int umidi_rtmsg(void *, int);
static void umidi_getinfo(void *, struct midi_info *);

static usbd_status alloc_pipe(struct umidi_endpoint *);
static void free_pipe(struct umidi_endpoint *);

static usbd_status alloc_all_endpoints(struct umidi_softc *);
static void free_all_endpoints(struct umidi_softc *);

static usbd_status alloc_all_jacks(struct umidi_softc *);
static void free_all_jacks(struct umidi_softc *);
static usbd_status bind_jacks_to_mididev(struct umidi_softc *,
					 struct umidi_jack *,
					 struct umidi_jack *,
					 struct umidi_mididev *);
static void unbind_jacks_from_mididev(struct umidi_mididev *);
static void unbind_all_jacks(struct umidi_softc *);
static usbd_status assign_all_jacks_automatically(struct umidi_softc *);
static usbd_status open_out_jack(struct umidi_jack *, void *,
				 void (*)(void *));
static usbd_status open_in_jack(struct umidi_jack *, void *,
				void (*)(void *, int));
static void close_out_jack(struct umidi_jack *);
static void close_in_jack(struct umidi_jack *);

static usbd_status attach_mididev(struct umidi_softc *, struct umidi_mididev *);
static usbd_status detach_mididev(struct umidi_mididev *, int);
static usbd_status deactivate_mididev(struct umidi_mididev *);
static usbd_status alloc_all_mididevs(struct umidi_softc *, int);
static void free_all_mididevs(struct umidi_softc *);
static usbd_status attach_all_mididevs(struct umidi_softc *);
static usbd_status detach_all_mididevs(struct umidi_softc *, int);
static usbd_status deactivate_all_mididevs(struct umidi_softc *);
static char *describe_mididev(struct umidi_mididev *);

#ifdef UMIDI_DEBUG
static void dump_sc(struct umidi_softc *);
static void dump_ep(struct umidi_endpoint *);
static void dump_jack(struct umidi_jack *);
#endif

static usbd_status start_input_transfer(struct umidi_endpoint *);
static usbd_status start_output_transfer(struct umidi_endpoint *);
static int out_jack_output(struct umidi_jack *, u_char *, int, int);
static void in_intr(usbd_xfer_handle, usbd_private_handle, usbd_status);
static void out_intr(usbd_xfer_handle, usbd_private_handle, usbd_status);
static void out_solicit(void *); /* struct umidi_endpoint* for softintr */


const struct midi_hw_if umidi_hw_if = {
	umidi_open,
	umidi_close,
	umidi_rtmsg,
	umidi_getinfo,
	0,		/* ioctl */
};

struct midi_hw_if_ext umidi_hw_if_ext = {
	.channel = umidi_channelmsg,
	.common  = umidi_commonmsg,
	.sysex   = umidi_sysex,
};

struct midi_hw_if_ext umidi_hw_if_mm = {
	.channel = umidi_channelmsg,
	.common  = umidi_commonmsg,
	.sysex   = umidi_sysex,
	.compress = 1,
};

USB_DECLARE_DRIVER(umidi);

USB_MATCH(umidi)
{
	USB_MATCH_START(umidi, uaa);
	usb_interface_descriptor_t *id;

	DPRINTFN(1,("umidi_match\n"));

	if (uaa->iface == NULL)
		return UMATCH_NONE;

	if (umidi_search_quirk(uaa->vendor, uaa->product, uaa->ifaceno))
		return UMATCH_IFACECLASS_IFACESUBCLASS;

	id = usbd_get_interface_descriptor(uaa->iface);
	if (id!=NULL &&
	    id->bInterfaceClass==UICLASS_AUDIO &&
	    id->bInterfaceSubClass==UISUBCLASS_MIDISTREAM)
		return UMATCH_IFACECLASS_IFACESUBCLASS;

	return UMATCH_NONE;
}

USB_ATTACH(umidi)
{
	usbd_status err;
	USB_ATTACH_START(umidi, sc, uaa);
	char *devinfop;

	DPRINTFN(1,("umidi_attach\n"));

	devinfop = usbd_devinfo_alloc(uaa->device, 0);
	printf("\n%s: %s\n", USBDEVNAME(sc->sc_dev), devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_iface = uaa->iface;
	sc->sc_udev = uaa->device;

	sc->sc_quirk =
	    umidi_search_quirk(uaa->vendor, uaa->product, uaa->ifaceno);
	printf("%s: ", USBDEVNAME(sc->sc_dev));
	umidi_print_quirk(sc->sc_quirk);


	err = alloc_all_endpoints(sc);
	if (err!=USBD_NORMAL_COMPLETION) {
		printf("%s: alloc_all_endpoints failed. (err=%d)\n",
		       USBDEVNAME(sc->sc_dev), err);
		goto error;
	}
	err = alloc_all_jacks(sc);
	if (err!=USBD_NORMAL_COMPLETION) {
		free_all_endpoints(sc);
		printf("%s: alloc_all_jacks failed. (err=%d)\n",
		       USBDEVNAME(sc->sc_dev), err);
		goto error;
	}
	printf("%s: out=%d, in=%d\n",
	       USBDEVNAME(sc->sc_dev),
	       sc->sc_out_num_jacks, sc->sc_in_num_jacks);

	err = assign_all_jacks_automatically(sc);
	if (err!=USBD_NORMAL_COMPLETION) {
		unbind_all_jacks(sc);
		free_all_jacks(sc);
		free_all_endpoints(sc);
		printf("%s: assign_all_jacks_automatically failed. (err=%d)\n",
		       USBDEVNAME(sc->sc_dev), err);
		goto error;
	}
	err = attach_all_mididevs(sc);
	if (err!=USBD_NORMAL_COMPLETION) {
		free_all_jacks(sc);
		free_all_endpoints(sc);
		printf("%s: attach_all_mididevs failed. (err=%d)\n",
		       USBDEVNAME(sc->sc_dev), err);
	}

#ifdef UMIDI_DEBUG
	dump_sc(sc);
#endif

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH,
			   sc->sc_udev, USBDEV(sc->sc_dev));

	USB_ATTACH_SUCCESS_RETURN;
error:
	printf("%s: disabled.\n", USBDEVNAME(sc->sc_dev));
	sc->sc_dying = 1;
	USB_ATTACH_ERROR_RETURN;
}

int
umidi_activate(device_ptr_t self, enum devact act)
{
	struct umidi_softc *sc = (struct umidi_softc *)self;

	switch (act) {
	case DVACT_ACTIVATE:
		DPRINTFN(1,("umidi_activate (activate)\n"));

		return EOPNOTSUPP;
		break;
	case DVACT_DEACTIVATE:
		DPRINTFN(1,("umidi_activate (deactivate)\n"));
		sc->sc_dying = 1;
		deactivate_all_mididevs(sc);
		break;
	}
	return 0;
}

USB_DETACH(umidi)
{
	USB_DETACH_START(umidi, sc);

	DPRINTFN(1,("umidi_detach\n"));

	sc->sc_dying = 1;
	detach_all_mididevs(sc, flags);
	free_all_mididevs(sc);
	free_all_jacks(sc);
	free_all_endpoints(sc);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	return 0;
}


/*
 * midi_if stuffs
 */
int
umidi_open(void *addr,
	   int flags,
	   void (*iintr)(void *, int),
	   void (*ointr)(void *),
	   void *arg)
{
	struct umidi_mididev *mididev = addr;
	struct umidi_softc *sc = mididev->sc;
	usbd_status err;

	DPRINTF(("umidi_open: sc=%p\n", sc));

	if (!sc)
		return ENXIO;
	if (mididev->opened)
		return EBUSY;
	if (sc->sc_dying)
		return EIO;

	mididev->opened = 1;
	mididev->flags = flags;
	if ((mididev->flags & FWRITE) && mididev->out_jack) {
		err = open_out_jack(mididev->out_jack, arg, ointr);
		if ( err != USBD_NORMAL_COMPLETION )
			goto bad;
	}
	if ((mididev->flags & FREAD) && mididev->in_jack) {
		err = open_in_jack(mididev->in_jack, arg, iintr);
		if ( err != USBD_NORMAL_COMPLETION
		&&   err != USBD_IN_PROGRESS )
			goto bad;
	}

	return 0;
bad:
	mididev->opened = 0;
	DPRINTF(("umidi_open: usbd_status %d\n", err));
	return USBD_IN_USE == err ? EBUSY : EIO;
}

void
umidi_close(void *addr)
{
	int s;
	struct umidi_mididev *mididev = addr;

	s = splusb();
	if ((mididev->flags & FWRITE) && mididev->out_jack)
		close_out_jack(mididev->out_jack);
	if ((mididev->flags & FREAD) && mididev->in_jack)
		close_in_jack(mididev->in_jack);
	mididev->opened = 0;
	splx(s);
}

int
umidi_channelmsg(void *addr, int status, int channel __unused, u_char *msg,
    int len)
{
	struct umidi_mididev *mididev = addr;

	if (!mididev->out_jack || !mididev->opened)
		return EIO;
	
	return out_jack_output(mididev->out_jack, msg, len, (status>>4)&0xf);
}

int
umidi_commonmsg(void *addr, int status __unused, u_char *msg, int len)
{
	struct umidi_mididev *mididev = addr;
	int cin;

	if (!mididev->out_jack || !mididev->opened)
		return EIO;

	switch ( len ) {
	case 1: cin = 5; break;
	case 2: cin = 2; break;
	case 3: cin = 3; break;
	default: return EIO; /* or gcc warns of cin uninitialized */
	}
	
	return out_jack_output(mididev->out_jack, msg, len, cin);
}

int
umidi_sysex(void *addr, u_char *msg, int len)
{
	struct umidi_mididev *mididev = addr;
	int cin;

	if (!mididev->out_jack || !mididev->opened)
		return EIO;

	switch ( len ) {
	case 1: cin = 5; break;
	case 2: cin = 6; break;
	case 3: cin = (msg[2] == 0xf7) ? 7 : 4; break;
	default: return EIO; /* or gcc warns of cin uninitialized */
	}
	
	return out_jack_output(mididev->out_jack, msg, len, cin);
}

int
umidi_rtmsg(void *addr, int d)
{
	struct umidi_mididev *mididev = addr;
	u_char msg = d;

	if (!mididev->out_jack || !mididev->opened)
		return EIO;

	return out_jack_output(mididev->out_jack, &msg, 1, 0xf);
}

void
umidi_getinfo(void *addr, struct midi_info *mi)
{
	struct umidi_mididev *mididev = addr;
	struct umidi_softc *sc = mididev->sc;
	int mm = UMQ_ISTYPE(sc, UMQ_TYPE_MIDIMAN_GARBLE);

	mi->name = mididev->label;
	mi->props = MIDI_PROP_OUT_INTR;
	if (mididev->in_jack)
		mi->props |= MIDI_PROP_CAN_INPUT;
	midi_register_hw_if_ext(mm? &umidi_hw_if_mm : &umidi_hw_if_ext);
}


/*
 * each endpoint stuffs
 */

/* alloc/free pipe */
static usbd_status
alloc_pipe(struct umidi_endpoint *ep)
{
	struct umidi_softc *sc = ep->sc;
	usbd_status err;
	usb_endpoint_descriptor_t *epd;
	
	epd = usbd_get_endpoint_descriptor(sc->sc_iface, ep->addr);
	/*
	 * For output, an improvement would be to have a buffer bigger than
	 * wMaxPacketSize by num_jacks-1 additional packet slots; that would
	 * allow out_solicit to fill the buffer to the full packet size in
	 * all cases. But to use usbd_alloc_buffer to get a slightly larger
	 * buffer would not be a good way to do that, because if the addition
	 * would make the buffer exceed USB_MEM_SMALL then a substantially
	 * larger block may be wastefully allocated. Some flavor of double
	 * buffering could serve the same purpose, but would increase the
	 * code complexity, so for now I will live with the current slight
	 * penalty of reducing max transfer size by (num_open-num_scheduled)
	 * packet slots.
	 */
	ep->buffer_size = UGETW(epd->wMaxPacketSize);
	ep->buffer_size -= ep->buffer_size % UMIDI_PACKET_SIZE;

	DPRINTF(("%s: alloc_pipe %p, buffer size %u\n",
	        USBDEVNAME(sc->sc_dev), ep, ep->buffer_size));
	ep->num_scheduled = 0;
	ep->this_schedule = 0;
	ep->next_schedule = 0;
	ep->soliciting = 0;
	ep->armed = 0;
	ep->xfer = usbd_alloc_xfer(sc->sc_udev);
	if (ep->xfer == NULL) {
	    err = USBD_NOMEM;
	    goto quit;
	}
	ep->buffer = usbd_alloc_buffer(ep->xfer, ep->buffer_size);
	if (ep->buffer == NULL) {
	    usbd_free_xfer(ep->xfer);
	    err = USBD_NOMEM;
	    goto quit;
	}
	ep->next_slot = ep->buffer;
	err = usbd_open_pipe(sc->sc_iface, ep->addr, 0, &ep->pipe);
	if (err)
	    usbd_free_xfer(ep->xfer);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	ep->solicit_cookie = softintr_establish(IPL_SOFTCLOCK,out_solicit,ep);
#endif
quit:
	return err;
}

static void
free_pipe(struct umidi_endpoint *ep)
{
	DPRINTF(("%s: free_pipe %p\n", USBDEVNAME(ep->sc->sc_dev), ep));
	usbd_abort_pipe(ep->pipe);
	usbd_close_pipe(ep->pipe);
	usbd_free_xfer(ep->xfer);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_disestablish(ep->solicit_cookie);
#endif
}


/* alloc/free the array of endpoint structures */

static usbd_status alloc_all_endpoints_fixed_ep(struct umidi_softc *);
static usbd_status alloc_all_endpoints_yamaha(struct umidi_softc *);
static usbd_status alloc_all_endpoints_genuine(struct umidi_softc *);

static usbd_status
alloc_all_endpoints(struct umidi_softc *sc)
{
	usbd_status err;
	struct umidi_endpoint *ep;
	int i;

	if (UMQ_ISTYPE(sc, UMQ_TYPE_FIXED_EP)) {
		err = alloc_all_endpoints_fixed_ep(sc);
	} else if (UMQ_ISTYPE(sc, UMQ_TYPE_YAMAHA)) {
		err = alloc_all_endpoints_yamaha(sc);
	} else {
		err = alloc_all_endpoints_genuine(sc);
	}
	if (err!=USBD_NORMAL_COMPLETION)
		return err;

	ep = sc->sc_endpoints;
	for (i=sc->sc_out_num_endpoints+sc->sc_in_num_endpoints; i>0; i--) {
		err = alloc_pipe(ep++);
		if (err!=USBD_NORMAL_COMPLETION) {
			for (; ep!=sc->sc_endpoints; ep--)
				free_pipe(ep-1);
			free(sc->sc_endpoints, M_USBDEV);
			sc->sc_endpoints = sc->sc_out_ep = sc->sc_in_ep = NULL;
			break;
		}
	}
	return err;
}

static void
free_all_endpoints(struct umidi_softc *sc)
{
	int i;
	for (i=0; i<sc->sc_in_num_endpoints+sc->sc_out_num_endpoints; i++)
	    free_pipe(&sc->sc_endpoints[i]);
	if (sc->sc_endpoints != NULL)
		free(sc->sc_endpoints, M_USBDEV);
	sc->sc_endpoints = sc->sc_out_ep = sc->sc_in_ep = NULL;
}

static usbd_status
alloc_all_endpoints_fixed_ep(struct umidi_softc *sc)
{
	usbd_status err;
	struct umq_fixed_ep_desc *fp;
	struct umidi_endpoint *ep;
	usb_endpoint_descriptor_t *epd;
	int i;

	fp = umidi_get_quirk_data_from_type(sc->sc_quirk,
					    UMQ_TYPE_FIXED_EP);
	sc->sc_out_num_jacks = 0;
	sc->sc_in_num_jacks = 0;
	sc->sc_out_num_endpoints = fp->num_out_ep;
	sc->sc_in_num_endpoints = fp->num_in_ep;
	sc->sc_endpoints = malloc(sizeof(*sc->sc_out_ep)*
				  (sc->sc_out_num_endpoints+
				   sc->sc_in_num_endpoints),
				  M_USBDEV, M_WAITOK);
	if (!sc->sc_endpoints) {
		return USBD_NOMEM;
	}
	sc->sc_out_ep = sc->sc_out_num_endpoints ? sc->sc_endpoints : NULL;
	sc->sc_in_ep =
	    sc->sc_in_num_endpoints ?
		sc->sc_endpoints+sc->sc_out_num_endpoints : NULL;

	ep = &sc->sc_out_ep[0];
	for (i=0; i<sc->sc_out_num_endpoints; i++) {
		epd = usbd_interface2endpoint_descriptor(
			sc->sc_iface,
			fp->out_ep[i].ep);
		if (!epd) {
			printf("%s: cannot get endpoint descriptor(out:%d)\n",
			       USBDEVNAME(sc->sc_dev), fp->out_ep[i].ep);
			err = USBD_INVAL;
			goto error;
		}
		if (UE_GET_XFERTYPE(epd->bmAttributes)!=UE_BULK ||
		    UE_GET_DIR(epd->bEndpointAddress)!=UE_DIR_OUT) {
			printf("%s: illegal endpoint(out:%d)\n",
			       USBDEVNAME(sc->sc_dev), fp->out_ep[i].ep);
			err = USBD_INVAL;
			goto error;
		}
		ep->sc = sc;
		ep->addr = epd->bEndpointAddress;
		ep->num_jacks = fp->out_ep[i].num_jacks;
		sc->sc_out_num_jacks += fp->out_ep[i].num_jacks;
		ep->num_open = 0;
		memset(ep->jacks, 0, sizeof(ep->jacks));
		ep++;
	}
	ep = &sc->sc_in_ep[0];
	for (i=0; i<sc->sc_in_num_endpoints; i++) {
		epd = usbd_interface2endpoint_descriptor(
			sc->sc_iface,
			fp->in_ep[i].ep);
		if (!epd) {
			printf("%s: cannot get endpoint descriptor(in:%d)\n",
			       USBDEVNAME(sc->sc_dev), fp->in_ep[i].ep);
			err = USBD_INVAL;
			goto error;
		}
		/*
		 * MIDISPORT_2X4 inputs on an interrupt rather than a bulk
		 * endpoint.  The existing input logic in this driver seems
		 * to work successfully if we just stop treating an interrupt
		 * endpoint as illegal (or the in_progress status we get on
		 * the initial transfer).  It does not seem necessary to
		 * actually use the interrupt flavor of alloc_pipe or make
		 * other serious rearrangements of logic.  I like that.
		 */
		switch ( UE_GET_XFERTYPE(epd->bmAttributes) ) {
		case UE_BULK:
		case UE_INTERRUPT:
			if ( UE_DIR_IN == UE_GET_DIR(epd->bEndpointAddress) )
				break;
			/*FALLTHROUGH*/
		default:
			printf("%s: illegal endpoint(in:%d)\n",
			       USBDEVNAME(sc->sc_dev), fp->in_ep[i].ep);
			err = USBD_INVAL;
			goto error;
		}

		ep->sc = sc;
		ep->addr = epd->bEndpointAddress;
		ep->num_jacks = fp->in_ep[i].num_jacks;
		sc->sc_in_num_jacks += fp->in_ep[i].num_jacks;
		ep->num_open = 0;
		memset(ep->jacks, 0, sizeof(ep->jacks));
		ep++;
	}

	return USBD_NORMAL_COMPLETION;
error:
	free(sc->sc_endpoints, M_USBDEV);
	sc->sc_endpoints = NULL;
	return err;
}

static usbd_status
alloc_all_endpoints_yamaha(struct umidi_softc *sc)
{
	/* This driver currently supports max 1in/1out bulk endpoints */
	usb_descriptor_t *desc;
	usb_endpoint_descriptor_t *epd;
	int out_addr, in_addr, i;
	int dir;
	size_t remain, descsize;

	sc->sc_out_num_jacks = sc->sc_in_num_jacks = 0;
	out_addr = in_addr = 0;

	/* detect endpoints */
	desc = TO_D(usbd_get_interface_descriptor(sc->sc_iface));
	for (i=(int)TO_IFD(desc)->bNumEndpoints-1; i>=0; i--) {
		epd = usbd_interface2endpoint_descriptor(sc->sc_iface, i);
		KASSERT(epd != NULL);
		if (UE_GET_XFERTYPE(epd->bmAttributes) == UE_BULK) {
			dir = UE_GET_DIR(epd->bEndpointAddress);
			if (dir==UE_DIR_OUT && !out_addr)
				out_addr = epd->bEndpointAddress;
			else if (dir==UE_DIR_IN && !in_addr)
				in_addr = epd->bEndpointAddress;
		}
	}
	desc = NEXT_D(desc);

	/* count jacks */
	if (!(desc->bDescriptorType==UDESC_CS_INTERFACE &&
	      desc->bDescriptorSubtype==UMIDI_MS_HEADER))
		return USBD_INVAL;
	remain = (size_t)UGETW(TO_CSIFD(desc)->wTotalLength) -
		(size_t)desc->bLength;
	desc = NEXT_D(desc);

	while (remain>=sizeof(usb_descriptor_t)) {
		descsize = desc->bLength;
		if (descsize>remain || descsize==0)
			break;
		if (desc->bDescriptorType==UDESC_CS_INTERFACE &&
		    remain>=UMIDI_JACK_DESCRIPTOR_SIZE) {
			if (desc->bDescriptorSubtype==UMIDI_OUT_JACK)
				sc->sc_out_num_jacks++;
			else if (desc->bDescriptorSubtype==UMIDI_IN_JACK)
				sc->sc_in_num_jacks++;
		}
		desc = NEXT_D(desc);
		remain-=descsize;
	}

	/* validate some parameters */
	if (sc->sc_out_num_jacks>UMIDI_MAX_EPJACKS)
		sc->sc_out_num_jacks = UMIDI_MAX_EPJACKS;
	if (sc->sc_in_num_jacks>UMIDI_MAX_EPJACKS)
		sc->sc_in_num_jacks = UMIDI_MAX_EPJACKS;
	if (sc->sc_out_num_jacks && out_addr) {
		sc->sc_out_num_endpoints = 1;
	} else {
		sc->sc_out_num_endpoints = 0;
		sc->sc_out_num_jacks = 0;
	}
	if (sc->sc_in_num_jacks && in_addr) {
		sc->sc_in_num_endpoints = 1;
	} else {
		sc->sc_in_num_endpoints = 0;
		sc->sc_in_num_jacks = 0;
	}
	sc->sc_endpoints = malloc(sizeof(struct umidi_endpoint)*
				  (sc->sc_out_num_endpoints+
				   sc->sc_in_num_endpoints),
				  M_USBDEV, M_WAITOK);
	if (!sc->sc_endpoints)
		return USBD_NOMEM;
	if (sc->sc_out_num_endpoints) {
		sc->sc_out_ep = sc->sc_endpoints;
		sc->sc_out_ep->sc = sc;
		sc->sc_out_ep->addr = out_addr;
		sc->sc_out_ep->num_jacks = sc->sc_out_num_jacks;
		sc->sc_out_ep->num_open = 0;
		memset(sc->sc_out_ep->jacks, 0, sizeof(sc->sc_out_ep->jacks));
	} else
		sc->sc_out_ep = NULL;

	if (sc->sc_in_num_endpoints) {
		sc->sc_in_ep = sc->sc_endpoints+sc->sc_out_num_endpoints;
		sc->sc_in_ep->sc = sc;
		sc->sc_in_ep->addr = in_addr;
		sc->sc_in_ep->num_jacks = sc->sc_in_num_jacks;
		sc->sc_in_ep->num_open = 0;
		memset(sc->sc_in_ep->jacks, 0, sizeof(sc->sc_in_ep->jacks));
	} else
		sc->sc_in_ep = NULL;

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
alloc_all_endpoints_genuine(struct umidi_softc *sc)
{
	usb_interface_descriptor_t *interface_desc;
	usb_config_descriptor_t *config_desc;
	usb_descriptor_t *desc;
	int num_ep;
	size_t remain, descsize;
	struct umidi_endpoint *p, *q, *lowest, *endep, tmpep;
	int epaddr;

	interface_desc = usbd_get_interface_descriptor(sc->sc_iface);
	num_ep = interface_desc->bNumEndpoints;
	sc->sc_endpoints = p = malloc(sizeof(struct umidi_endpoint) * num_ep,
				      M_USBDEV, M_WAITOK);
	if (!p)
		return USBD_NOMEM;

	sc->sc_out_num_jacks = sc->sc_in_num_jacks = 0;
	sc->sc_out_num_endpoints = sc->sc_in_num_endpoints = 0;
	epaddr = -1;

	/* get the list of endpoints for midi stream */
	config_desc = usbd_get_config_descriptor(sc->sc_udev);
	desc = (usb_descriptor_t *) config_desc;
	remain = (size_t)UGETW(config_desc->wTotalLength);
	while (remain>=sizeof(usb_descriptor_t)) {
		descsize = desc->bLength;
		if (descsize>remain || descsize==0)
			break;
		if (desc->bDescriptorType==UDESC_ENDPOINT &&
		    remain>=USB_ENDPOINT_DESCRIPTOR_SIZE &&
		    UE_GET_XFERTYPE(TO_EPD(desc)->bmAttributes) == UE_BULK) {
			epaddr = TO_EPD(desc)->bEndpointAddress;
		} else if (desc->bDescriptorType==UDESC_CS_ENDPOINT &&
			   remain>=UMIDI_CS_ENDPOINT_DESCRIPTOR_SIZE &&
			   epaddr!=-1) {
			if (num_ep>0) {
				num_ep--;
				p->sc = sc;
				p->addr = epaddr;
				p->num_jacks = TO_CSEPD(desc)->bNumEmbMIDIJack;
				if (UE_GET_DIR(epaddr)==UE_DIR_OUT) {
					sc->sc_out_num_endpoints++;
					sc->sc_out_num_jacks += p->num_jacks;
				} else {
					sc->sc_in_num_endpoints++;
					sc->sc_in_num_jacks += p->num_jacks;
				}
				p++;
			}
		} else
			epaddr = -1;
		desc = NEXT_D(desc);
		remain-=descsize;
	}

	/* sort endpoints */
	num_ep = sc->sc_out_num_endpoints + sc->sc_in_num_endpoints;
	p = sc->sc_endpoints;
	endep = p + num_ep;
	while (p<endep) {
		lowest = p;
		for (q=p+1; q<endep; q++) {
			if ((UE_GET_DIR(lowest->addr)==UE_DIR_IN &&
			     UE_GET_DIR(q->addr)==UE_DIR_OUT) ||
			    ((UE_GET_DIR(lowest->addr)==
			      UE_GET_DIR(q->addr)) &&
			     (UE_GET_ADDR(lowest->addr)>
			      UE_GET_ADDR(q->addr))))
				lowest = q;
		}
		if (lowest != p) {
			memcpy((void *)&tmpep, (void *)p, sizeof(tmpep));
			memcpy((void *)p, (void *)lowest, sizeof(tmpep));
			memcpy((void *)lowest, (void *)&tmpep, sizeof(tmpep));
		}
		p->num_open = 0;
		p++;
	}

	sc->sc_out_ep = sc->sc_out_num_endpoints ? sc->sc_endpoints : NULL;
	sc->sc_in_ep =
	    sc->sc_in_num_endpoints ?
		sc->sc_endpoints+sc->sc_out_num_endpoints : NULL;

	return USBD_NORMAL_COMPLETION;
}


/*
 * jack stuffs
 */

static usbd_status
alloc_all_jacks(struct umidi_softc *sc)
{
	int i, j;
	struct umidi_endpoint *ep;
	struct umidi_jack *jack;
	unsigned char *cn_spec;
	
	if (UMQ_ISTYPE(sc, UMQ_TYPE_CN_SEQ_PER_EP))
		sc->cblnums_global = 0;
	else if (UMQ_ISTYPE(sc, UMQ_TYPE_CN_SEQ_GLOBAL))
		sc->cblnums_global = 1;
	else {
		/*
		 * I don't think this default is correct, but it preserves
		 * the prior behavior of the code. That's why I defined two
		 * complementary quirks. Any device for which the default
		 * behavior is wrong can be made to work by giving it an
		 * explicit quirk, and if a pattern ever develops (as I suspect
		 * it will) that a lot of otherwise standard USB MIDI devices
		 * need the CN_SEQ_PER_EP "quirk," then this default can be
		 * changed to 0, and the only devices that will break are those
		 * listing neither quirk, and they'll easily be fixed by giving
		 * them the CN_SEQ_GLOBAL quirk.
		 */
		sc->cblnums_global = 1;
	}
	
	if (UMQ_ISTYPE(sc, UMQ_TYPE_CN_FIXED))
		cn_spec = umidi_get_quirk_data_from_type(sc->sc_quirk,
					    		 UMQ_TYPE_CN_FIXED);
	else
		cn_spec = NULL;

	/* allocate/initialize structures */
	sc->sc_jacks =
	    malloc(sizeof(*sc->sc_out_jacks)*(sc->sc_in_num_jacks+
					      sc->sc_out_num_jacks),
		   M_USBDEV, M_WAITOK);
	if (!sc->sc_jacks)
		return USBD_NOMEM;
	sc->sc_out_jacks =
	    sc->sc_out_num_jacks ? sc->sc_jacks : NULL;
	sc->sc_in_jacks =
	    sc->sc_in_num_jacks ? sc->sc_jacks+sc->sc_out_num_jacks : NULL;

	jack = &sc->sc_out_jacks[0];
	for (i=0; i<sc->sc_out_num_jacks; i++) {
		jack->opened = 0;
		jack->binded = 0;
		jack->arg = NULL;
		jack->u.out.intr = NULL;
		jack->midiman_ppkt = NULL;
		if ( sc->cblnums_global )
			jack->cable_number = i;
		jack++;
	}
	jack = &sc->sc_in_jacks[0];
	for (i=0; i<sc->sc_in_num_jacks; i++) {
		jack->opened = 0;
		jack->binded = 0;
		jack->arg = NULL;
		jack->u.in.intr = NULL;
		if ( sc->cblnums_global )
			jack->cable_number = i;
		jack++;
	}

	/* assign each jacks to each endpoints */
	jack = &sc->sc_out_jacks[0];
	ep = &sc->sc_out_ep[0];
	for (i=0; i<sc->sc_out_num_endpoints; i++) {
		for (j=0; j<ep->num_jacks; j++) {
			jack->endpoint = ep;
			if ( cn_spec != NULL )
				jack->cable_number = *cn_spec++;
			else if ( !sc->cblnums_global )
				jack->cable_number = j;
			ep->jacks[jack->cable_number] = jack;
			jack++;
		}
		ep++;
	}
	jack = &sc->sc_in_jacks[0];
	ep = &sc->sc_in_ep[0];
	for (i=0; i<sc->sc_in_num_endpoints; i++) {
		for (j=0; j<ep->num_jacks; j++) {
			jack->endpoint = ep;
			if ( cn_spec != NULL )
				jack->cable_number = *cn_spec++;
			else if ( !sc->cblnums_global )
				jack->cable_number = j;
			ep->jacks[jack->cable_number] = jack;
			jack++;
		}
		ep++;
	}

	return USBD_NORMAL_COMPLETION;
}

static void
free_all_jacks(struct umidi_softc *sc)
{
	int s;

	s = splaudio();
	if (sc->sc_out_jacks) {
		free(sc->sc_jacks, M_USBDEV);
		sc->sc_jacks = sc->sc_in_jacks = sc->sc_out_jacks = NULL;
	}
	splx(s);
}

static usbd_status
bind_jacks_to_mididev(struct umidi_softc *sc __unused,
		      struct umidi_jack *out_jack,
		      struct umidi_jack *in_jack,
		      struct umidi_mididev *mididev)
{
	if ((out_jack && out_jack->binded) || (in_jack && in_jack->binded))
		return USBD_IN_USE;
	if (mididev->out_jack || mididev->in_jack)
		return USBD_IN_USE;

	if (out_jack)
		out_jack->binded = 1;
	if (in_jack)
		in_jack->binded = 1;
	mididev->in_jack = in_jack;
	mididev->out_jack = out_jack;

	return USBD_NORMAL_COMPLETION;
}

static void
unbind_jacks_from_mididev(struct umidi_mididev *mididev)
{
	if ((mididev->flags & FWRITE) && mididev->out_jack)
		close_out_jack(mididev->out_jack);
	if ((mididev->flags & FREAD) && mididev->in_jack)
		close_in_jack(mididev->in_jack);

	if (mididev->out_jack)
		mididev->out_jack->binded = 0;
	if (mididev->in_jack)
		mididev->in_jack->binded = 0;
	mididev->out_jack = mididev->in_jack = NULL;
}

static void
unbind_all_jacks(struct umidi_softc *sc)
{
	int i;

	if (sc->sc_mididevs)
		for (i=0; i<sc->sc_num_mididevs; i++) {
			unbind_jacks_from_mididev(&sc->sc_mididevs[i]);
		}
}

static usbd_status
assign_all_jacks_automatically(struct umidi_softc *sc)
{
	usbd_status err;
	int i;
	struct umidi_jack *out, *in;
	signed char *asg_spec;

	err =
	    alloc_all_mididevs(sc,
			       max(sc->sc_out_num_jacks, sc->sc_in_num_jacks));
	if (err!=USBD_NORMAL_COMPLETION)
		return err;

	if ( UMQ_ISTYPE(sc, UMQ_TYPE_MD_FIXED))
		asg_spec = umidi_get_quirk_data_from_type(sc->sc_quirk,
					    		  UMQ_TYPE_MD_FIXED);
	else
		asg_spec = NULL;

	for (i=0; i<sc->sc_num_mididevs; i++) {
		if ( asg_spec != NULL ) {
			if ( *asg_spec == -1 )
				out = NULL;
			else
				out = &sc->sc_out_jacks[*asg_spec];
			++ asg_spec;
			if ( *asg_spec == -1 )
				in = NULL;
			else
				in = &sc->sc_in_jacks[*asg_spec];
			++ asg_spec;
		} else {
			out = (i<sc->sc_out_num_jacks) ? &sc->sc_out_jacks[i]
			                               : NULL;
			in = (i<sc->sc_in_num_jacks) ? &sc->sc_in_jacks[i]
						     : NULL;
		}
		err = bind_jacks_to_mididev(sc, out, in, &sc->sc_mididevs[i]);
		if (err!=USBD_NORMAL_COMPLETION) {
			free_all_mididevs(sc);
			return err;
		}
	}

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
open_out_jack(struct umidi_jack *jack, void *arg, void (*intr)(void *))
{
	struct umidi_endpoint *ep = jack->endpoint;
	umidi_packet_bufp end;
	int s;
	int err;

	if (jack->opened)
		return USBD_IN_USE;

	jack->arg = arg;
	jack->u.out.intr = intr;
	jack->midiman_ppkt = NULL;
	end = ep->buffer + ep->buffer_size / sizeof *ep->buffer;
	s = splusb();
	jack->opened = 1;
	ep->num_open++;
	/*
	 * out_solicit maintains an invariant that there will always be
	 * (num_open - num_scheduled) slots free in the buffer. as we have
	 * just incremented num_open, the buffer may be too full to satisfy
	 * the invariant until a transfer completes, for which we must wait.
	 */
	while ( end - ep->next_slot < ep->num_open - ep->num_scheduled ) {
		err = tsleep(ep, PWAIT|PCATCH, "umi op", mstohz(10));
		if ( err ) {
			ep->num_open--;
			jack->opened = 0;
			splx(s);
			return USBD_IOERROR;
		}
	}
	splx(s);

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
open_in_jack(struct umidi_jack *jack, void *arg, void (*intr)(void *, int))
{
	usbd_status err = USBD_NORMAL_COMPLETION;
	struct umidi_endpoint *ep = jack->endpoint;

	if (jack->opened)
		return USBD_IN_USE;

	jack->arg = arg;
	jack->u.in.intr = intr;
	jack->opened = 1;
	if (ep->num_open++==0 && UE_GET_DIR(ep->addr)==UE_DIR_IN) {
		err = start_input_transfer(ep);
		if (err != USBD_NORMAL_COMPLETION &&
		    err != USBD_IN_PROGRESS) {
			ep->num_open--;
		}
	}

	return err;
}

static void
close_out_jack(struct umidi_jack *jack)
{
	struct umidi_endpoint *ep;
	int s;
	u_int16_t mask;
	int err;

	if (jack->opened) {
		ep = jack->endpoint;
		mask = 1 << (jack->cable_number);
		s = splusb();
		while ( mask & (ep->this_schedule | ep->next_schedule) ) {
			err = tsleep(ep, PWAIT|PCATCH, "umi dr", mstohz(10));
			if ( err )
				break;
		}
		jack->opened = 0;
		jack->endpoint->num_open--;
		ep->this_schedule &= ~mask;
		ep->next_schedule &= ~mask;
		splx(s);
	}
}

static void
close_in_jack(struct umidi_jack *jack)
{
	if (jack->opened) {
		jack->opened = 0;
		if (--jack->endpoint->num_open == 0) {
		    usbd_abort_pipe(jack->endpoint->pipe);
		}
	}
}

static usbd_status
attach_mididev(struct umidi_softc *sc, struct umidi_mididev *mididev)
{
	if (mididev->sc)
		return USBD_IN_USE;

	mididev->sc = sc;
	
	mididev->label = describe_mididev(mididev);

	mididev->mdev = midi_attach_mi(&umidi_hw_if, mididev, &sc->sc_dev);

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
detach_mididev(struct umidi_mididev *mididev, int flags)
{
	if (!mididev->sc)
		return USBD_NO_ADDR;

	if (mididev->opened) {
		umidi_close(mididev);
	}
	unbind_jacks_from_mididev(mididev);

	if (mididev->mdev)
		config_detach(mididev->mdev, flags);
	
	if (NULL != mididev->label) {
		free(mididev->label, M_USBDEV);
		mididev->label = NULL;
	}

	mididev->sc = NULL;

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
deactivate_mididev(struct umidi_mididev *mididev)
{
	if (mididev->out_jack)
		mididev->out_jack->binded = 0;
	if (mididev->in_jack)
		mididev->in_jack->binded = 0;
	config_deactivate(mididev->mdev);

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
alloc_all_mididevs(struct umidi_softc *sc, int nmidi)
{
	sc->sc_num_mididevs = nmidi;
	sc->sc_mididevs = malloc(sizeof(*sc->sc_mididevs)*nmidi,
				 M_USBDEV, M_WAITOK|M_ZERO);
	if (!sc->sc_mididevs)
		return USBD_NOMEM;

	return USBD_NORMAL_COMPLETION;
}

static void
free_all_mididevs(struct umidi_softc *sc)
{
	sc->sc_num_mididevs = 0;
	if (sc->sc_mididevs)
		free(sc->sc_mididevs, M_USBDEV);
}

static usbd_status
attach_all_mididevs(struct umidi_softc *sc)
{
	usbd_status err;
	int i;

	if (sc->sc_mididevs)
		for (i=0; i<sc->sc_num_mididevs; i++) {
			err = attach_mididev(sc, &sc->sc_mididevs[i]);
			if (err!=USBD_NORMAL_COMPLETION)
				return err;
		}

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
detach_all_mididevs(struct umidi_softc *sc, int flags)
{
	usbd_status err;
	int i;

	if (sc->sc_mididevs)
		for (i=0; i<sc->sc_num_mididevs; i++) {
			err = detach_mididev(&sc->sc_mididevs[i], flags);
			if (err!=USBD_NORMAL_COMPLETION)
				return err;
		}

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
deactivate_all_mididevs(struct umidi_softc *sc)
{
	usbd_status err;
	int i;

	if (sc->sc_mididevs)
		for (i=0; i<sc->sc_num_mididevs; i++) {
			err = deactivate_mididev(&sc->sc_mididevs[i]);
			if (err!=USBD_NORMAL_COMPLETION)
				return err;
		}

	return USBD_NORMAL_COMPLETION;
}

/*
 * TODO: the 0-based cable numbers will often not match the labeling of the
 * equipment. Ideally:
 *  For class-compliant devices: get the iJack string from the jack descriptor.
 *  Otherwise:
 *  - support a DISPLAY_BASE_CN quirk (add the value to each internal cable
 *    number for display)
 *  - support an array quirk explictly giving a char * for each jack.
 * For now, you get 0-based cable numbers. If there are multiple endpoints and
 * the CNs are not globally unique, each is shown with its associated endpoint
 * address in hex also. That should not be necessary when using iJack values
 * or a quirk array.
 */
static char *
describe_mididev(struct umidi_mididev *md)
{
	char in_label[16];
	char out_label[16];
	char *unit_label;
	char *final_label;
	struct umidi_softc *sc;
	int show_ep_in;
	int show_ep_out;
	size_t len;
	
	sc = md->sc;
	show_ep_in  = sc-> sc_in_num_endpoints > 1 && !sc->cblnums_global;
	show_ep_out = sc->sc_out_num_endpoints > 1 && !sc->cblnums_global;
	
	if ( NULL != md->in_jack )
		snprintf(in_label, sizeof in_label,
		    show_ep_in ? "<%d(%x) " : "<%d ",
		    md->in_jack->cable_number,
		    md->in_jack->endpoint->addr);
	else
		in_label[0] = '\0';
	
	if ( NULL != md->out_jack )
		snprintf(out_label, sizeof out_label,
		    show_ep_out ? ">%d(%x) " : ">%d ",
		    md->out_jack->cable_number,
		    md->out_jack->endpoint->addr);
	else
		in_label[0] = '\0';

	unit_label = USBDEVNAME(sc->sc_dev);
	
	len = strlen(in_label) + strlen(out_label) + strlen(unit_label) + 4;
	
	final_label = malloc(len, M_USBDEV, M_WAITOK);
	
	snprintf(final_label, len, "%s%son %s",
	    in_label, out_label, unit_label);

	return final_label;
}

#ifdef UMIDI_DEBUG
static void
dump_sc(struct umidi_softc *sc)
{
	int i;

	DPRINTFN(10, ("%s: dump_sc\n", USBDEVNAME(sc->sc_dev)));
	for (i=0; i<sc->sc_out_num_endpoints; i++) {
		DPRINTFN(10, ("\tout_ep(%p):\n", &sc->sc_out_ep[i]));
		dump_ep(&sc->sc_out_ep[i]);
	}
	for (i=0; i<sc->sc_in_num_endpoints; i++) {
		DPRINTFN(10, ("\tin_ep(%p):\n", &sc->sc_in_ep[i]));
		dump_ep(&sc->sc_in_ep[i]);
	}
}

static void
dump_ep(struct umidi_endpoint *ep)
{
	int i;
	for (i=0; i<UMIDI_MAX_EPJACKS; i++) {
		if (NULL==ep->jacks[i])
			continue;
		DPRINTFN(10, ("\t\tjack[%d]:%p:\n", i, ep->jacks[i]));
		dump_jack(ep->jacks[i]);
	}
}
static void
dump_jack(struct umidi_jack *jack)
{
	DPRINTFN(10, ("\t\t\tep=%p\n",
		      jack->endpoint));
}

#endif /* UMIDI_DEBUG */



/*
 * MUX MIDI PACKET
 */

static const int packet_length[16] = {
	/*0*/	-1,
	/*1*/	-1,
	/*2*/	2,
	/*3*/	3,
	/*4*/	3,
	/*5*/	1,
	/*6*/	2,
	/*7*/	3,
	/*8*/	3,
	/*9*/	3,
	/*A*/	3,
	/*B*/	3,
	/*C*/	2,
	/*D*/	2,
	/*E*/	3,
	/*F*/	1,
};

#define	GET_CN(p)		(((unsigned char)(p)>>4)&0x0F)
#define GET_CIN(p)		((unsigned char)(p)&0x0F)
#define MIX_CN_CIN(cn, cin) \
	((unsigned char)((((unsigned char)(cn)&0x0F)<<4)| \
			  ((unsigned char)(cin)&0x0F)))

static usbd_status
start_input_transfer(struct umidi_endpoint *ep)
{
	usbd_setup_xfer(ep->xfer, ep->pipe,
			(usbd_private_handle)ep,
			ep->buffer, ep->buffer_size,
			USBD_SHORT_XFER_OK | USBD_NO_COPY,
                        USBD_NO_TIMEOUT, in_intr);
	return usbd_transfer(ep->xfer);
}

static usbd_status
start_output_transfer(struct umidi_endpoint *ep)
{
	usbd_status rv;
	u_int32_t length;
	int i;
	
	length = (ep->next_slot - ep->buffer) * sizeof *ep->buffer;
	DPRINTFN(200,("umidi out transfer: start %p end %p length %u\n",
	    ep->buffer, ep->next_slot, length));
	usbd_setup_xfer(ep->xfer, ep->pipe,
			(usbd_private_handle)ep,
			ep->buffer, length,
			USBD_NO_COPY, USBD_NO_TIMEOUT, out_intr);
	rv = usbd_transfer(ep->xfer);
	
	/*
	 * Once the transfer is scheduled, no more adding to partial
	 * packets within it.
	 */
	if (UMQ_ISTYPE(ep->sc, UMQ_TYPE_MIDIMAN_GARBLE)) {
		for (i=0; i<UMIDI_MAX_EPJACKS; ++i)
			if (NULL != ep->jacks[i])
				ep->jacks[i]->midiman_ppkt = NULL;
	}
	
	return rv;
}

#ifdef UMIDI_DEBUG
#define DPR_PACKET(dir, sc, p)						\
if ((unsigned char)(p)[1]!=0xFE)				\
	DPRINTFN(500,							\
		 ("%s: umidi packet(" #dir "): %02X %02X %02X %02X\n",	\
		  USBDEVNAME(sc->sc_dev),				\
		  (unsigned char)(p)[0],			\
		  (unsigned char)(p)[1],			\
		  (unsigned char)(p)[2],			\
		  (unsigned char)(p)[3]));
#else
#define DPR_PACKET(dir, sc, p)
#endif

/*
 * A 4-byte Midiman packet superficially resembles a 4-byte USB MIDI packet
 * with the cable number and length in the last byte instead of the first,
 * but there the resemblance ends. Where a USB MIDI packet is a semantic
 * unit, a Midiman packet is just a wrapper for 1 to 3 bytes of raw MIDI
 * with a cable nybble and a length nybble (which, unlike the CIN of a
 * real USB MIDI packet, has no semantics at all besides the length).
 * A packet received from a Midiman may contain part of a MIDI message,
 * more than one MIDI message, or parts of more than one MIDI message. A
 * three-byte MIDI message may arrive in three packets of data length 1, and
 * running status may be used. Happily, the midi(4) driver above us will put
 * it all back together, so the only cost is in USB bandwidth. The device
 * has an easier time with what it receives from us: we'll pack messages in
 * and across packets, but filling the packets whenever possible and,
 * as midi(4) hands us a complete message at a time, we'll never send one
 * in a dribble of short packets.
 */

static int
out_jack_output(struct umidi_jack *out_jack, u_char *src, int len, int cin)
{
	struct umidi_endpoint *ep = out_jack->endpoint;
	struct umidi_softc *sc = ep->sc;
	unsigned char *packet;
	int s;
	int plen;
	int poff;

	if (sc->sc_dying)
		return EIO;

	if (!out_jack->opened)
		return ENODEV; /* XXX as it was, is this the right errno? */

#ifdef UMIDI_DEBUG
	if ( umididebug >= 100 )
		microtime(&umidi_tv);
#endif
	DPRINTFN(100, ("umidi out: %lu.%06lus ep=%p cn=%d len=%d cin=%#x\n",
	    umidi_tv.tv_sec%100, umidi_tv.tv_usec,
	    ep, out_jack->cable_number, len, cin));
	
	s = splusb();
	packet = *ep->next_slot++;
	KASSERT(ep->buffer_size >=
	    (ep->next_slot - ep->buffer) * sizeof *ep->buffer);
	memset(packet, 0, UMIDI_PACKET_SIZE);
	if (UMQ_ISTYPE(sc, UMQ_TYPE_MIDIMAN_GARBLE)) {
		if (NULL != out_jack->midiman_ppkt) { /* fill out a prev pkt */
			poff = 0x0f & (out_jack->midiman_ppkt[3]);
			plen = 3 - poff;
			if (plen > len)
				plen = len;
			memcpy(out_jack->midiman_ppkt+poff, src, plen);
			src += plen;
			len -= plen;
			plen += poff;
			out_jack->midiman_ppkt[3] =
			    MIX_CN_CIN(out_jack->cable_number, plen);
			DPR_PACKET(out+, sc, out_jack->midiman_ppkt);
			if (3 == plen)
				out_jack->midiman_ppkt = NULL; /* no more */
		}
		if (0 == len)
			ep->next_slot--; /* won't be needed, nevermind */
		else {
			memcpy(packet, src, len);
			packet[3] = MIX_CN_CIN(out_jack->cable_number, len);
			DPR_PACKET(out, sc, packet);
			if (len < 3)
				out_jack->midiman_ppkt = packet;
		}
	} else { /* the nice simple USB class-compliant case */
		packet[0] = MIX_CN_CIN(out_jack->cable_number, cin);
		memcpy(packet+1, src, len);
		DPR_PACKET(out, sc, packet);
	}
	ep->next_schedule |= 1<<(out_jack->cable_number);
	++ ep->num_scheduled;
	if ( !ep->armed  &&  !ep->soliciting ) {
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		/*
		 * It would be bad to call out_solicit directly here (the
		 * caller need not be reentrant) but a soft interrupt allows
		 * solicit to run immediately the caller exits its critical
		 * section, and if the caller has more to write we can get it
		 * before starting the USB transfer, and send a longer one.
		 */
		ep->soliciting = 1;
		softintr_schedule(ep->solicit_cookie);
#else
		/*
		 * This alternative is a little less desirable, because if the
		 * writer has several messages to go at once, the first will go
		 * in a USB frame all to itself, and the rest in a full-size
		 * transfer one frame later (solicited on the first frame's
		 * completion interrupt). But it's simple.
		 */
		ep->armed = (USBD_IN_PROGRESS == start_output_transfer(ep));
#endif
	}
	splx(s);
	
	return 0;
}

static void
in_intr(usbd_xfer_handle xfer, usbd_private_handle priv,
    usbd_status status __unused)
{
	int cn, len, i;
	struct umidi_endpoint *ep = (struct umidi_endpoint *)priv;
	struct umidi_jack *jack;
	unsigned char *packet;
	umidi_packet_bufp slot;
	umidi_packet_bufp end;
	unsigned char *data;
	u_int32_t count;

	if (ep->sc->sc_dying || !ep->num_open)
		return;

	usbd_get_xfer_status(xfer, NULL, NULL, &count, NULL);
        if ( 0 == count % UMIDI_PACKET_SIZE ) {
		DPRINTFN(200,("%s: input endpoint %p transfer length %u\n",
			     USBDEVNAME(ep->sc->sc_dev), ep, count));
        } else {
                DPRINTF(("%s: input endpoint %p odd transfer length %u\n",
                        USBDEVNAME(ep->sc->sc_dev), ep, count));
        }
	
	slot = ep->buffer;
	end = slot + count / sizeof *slot;

	for ( packet = *slot; slot < end; packet = *++slot ) {
	
		if ( UMQ_ISTYPE(ep->sc, UMQ_TYPE_MIDIMAN_GARBLE) ) {
			cn = (0xf0&(packet[3]))>>4;
			len = 0x0f&(packet[3]);
			data = packet;
		} else {
			cn = GET_CN(packet[0]);
			len = packet_length[GET_CIN(packet[0])];
			data = packet + 1;
		}
		/* 0 <= cn <= 15 by inspection of above code */
		if (!(jack = ep->jacks[cn]) || cn != jack->cable_number) {
			DPRINTF(("%s: stray input endpoint %p cable %d len %d: "
			         "%02X %02X %02X (try CN_SEQ quirk?)\n",
				 USBDEVNAME(ep->sc->sc_dev), ep, cn, len,
				 (unsigned)data[0],
				 (unsigned)data[1],
				 (unsigned)data[2]));
			return;
		}

		if (!jack->binded || !jack->opened)
			continue;

		DPRINTFN(500,("%s: input endpoint %p cable %d len %d: "
		             "%02X %02X %02X\n",
			     USBDEVNAME(ep->sc->sc_dev), ep, cn, len,
			     (unsigned)data[0],
			     (unsigned)data[1],
			     (unsigned)data[2]));

		if (jack->u.in.intr) {
			for (i=0; i<len; i++) {
				(*jack->u.in.intr)(jack->arg, data[i]);
			}
		}

	}

	(void)start_input_transfer(ep);
}

static void
out_intr(usbd_xfer_handle xfer, usbd_private_handle priv,
    usbd_status status __unused)
{
	struct umidi_endpoint *ep = (struct umidi_endpoint *)priv;
	struct umidi_softc *sc = ep->sc;
	u_int32_t count;

	if (sc->sc_dying)
		return;

#ifdef UMIDI_DEBUG
	if ( umididebug >= 200 )
		microtime(&umidi_tv);
#endif
	usbd_get_xfer_status(xfer, NULL, NULL, &count, NULL);
        if ( 0 == count % UMIDI_PACKET_SIZE ) {
		DPRINTFN(200,("%s: %lu.%06lus out ep %p xfer length %u\n",
			     USBDEVNAME(ep->sc->sc_dev),
			     umidi_tv.tv_sec%100, umidi_tv.tv_usec, ep, count));
        } else {
                DPRINTF(("%s: output endpoint %p odd transfer length %u\n",
                        USBDEVNAME(ep->sc->sc_dev), ep, count));
        }
	count /= UMIDI_PACKET_SIZE;
	
	/*
	 * If while the transfer was pending we buffered any new messages,
	 * move them to the start of the buffer.
	 */
	ep->next_slot -= count;
	if ( ep->buffer < ep->next_slot ) {
		memcpy(ep->buffer, ep->buffer + count,
		       (char *)ep->next_slot - (char *)ep->buffer);
	}
	wakeup(ep);
	/*
	 * Do not want anyone else to see armed <- 0 before soliciting <- 1.
	 * Running at splusb so the following should happen to be safe.
	 */
	ep->armed = 0;
	if ( !ep->soliciting ) {
		ep->soliciting = 1;
		out_solicit(ep);
	}
}

/*
 * A jack on which we have received a packet must be called back on its
 * out.intr handler before it will send us another; it is considered
 * 'scheduled'. It is nice and predictable - as long as it is scheduled,
 * we need no extra buffer space for it.
 *
 * In contrast, a jack that is open but not scheduled may supply us a packet
 * at any time, driven by the top half, and we must be able to accept it, no
 * excuses. So we must ensure that at any point in time there are at least
 * (num_open - num_scheduled) slots free.
 *
 * As long as there are more slots free than that minimum, we can loop calling
 * scheduled jacks back on their "interrupt" handlers, soliciting more
 * packets, starting the USB transfer only when the buffer space is down to
 * the minimum or no jack has any more to send.
 */
static void
out_solicit(void *arg)
{
	struct umidi_endpoint *ep = arg;
	int s;
	umidi_packet_bufp end;
	u_int16_t which;
	struct umidi_jack *jack;
	
	end = ep->buffer + ep->buffer_size / sizeof *ep->buffer;
	
	for ( ;; ) {
		s = splusb();
		if ( end - ep->next_slot <= ep->num_open - ep->num_scheduled )
			break; /* at splusb */
		if ( ep->this_schedule == 0 ) {
			if ( ep->next_schedule == 0 )
				break; /* at splusb */
			ep->this_schedule = ep->next_schedule;
			ep->next_schedule = 0;
		}
		/*
		 * At least one jack is scheduled. Find and mask off the least
		 * set bit in this_schedule and decrement num_scheduled.
		 * Convert mask to bit index to find the corresponding jack,
		 * and call its intr handler. If it has a message, it will call
		 * back one of the output methods, which will set its bit in
		 * next_schedule (not copied into this_schedule until the
		 * latter is empty). In this way we round-robin the jacks that
		 * have messages to send, until the buffer is as full as we
		 * dare, and then start a transfer.
		 */
		which = ep->this_schedule;
		which &= (~which)+1; /* now mask of least set bit */
		ep->this_schedule &= ~which;
		-- ep->num_scheduled;
		splx(s);

		-- which; /* now 1s below mask - count 1s to get index */
		which -= ((which >> 1) & 0x5555);/* SWAR credit aggregate.org */
		which = (((which >> 2) & 0x3333) + (which & 0x3333));
		which = (((which >> 4) + which) & 0x0f0f);
		which +=  (which >> 8);
		which &= 0x1f; /* the bit index a/k/a jack number */
		
		jack = ep->jacks[which];
		if (jack->u.out.intr)
			(*jack->u.out.intr)(jack->arg);
	}
	/* splusb at loop exit */
	if ( !ep->armed  &&  ep->next_slot > ep->buffer )
		ep->armed = (USBD_IN_PROGRESS == start_output_transfer(ep));
	ep->soliciting = 0;
	splx(s);
}
