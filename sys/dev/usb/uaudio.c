/*	$NetBSD: uaudio.c,v 1.181 2023/04/30 08:35:52 mlelstv Exp $	*/

/*
 * Copyright (c) 1999, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology, and Matthew R. Green (mrg@eterna.com.au).
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
 * USB audio specs: http://www.usb.org/developers/docs/devclass_docs/audio10.pdf
 *                  http://www.usb.org/developers/docs/devclass_docs/frmts10.pdf
 *                  http://www.usb.org/developers/docs/devclass_docs/termt10.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uaudio.c,v 1.181 2023/04/30 08:35:52 mlelstv Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/reboot.h>		/* for bootverbose */
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/poll.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/sysctl.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/usbdevs.h>

#include <dev/usb/uaudioreg.h>

/* #define UAUDIO_DEBUG */
#define UAUDIO_MULTIPLE_ENDPOINTS
#ifdef UAUDIO_DEBUG
#define DPRINTF(x,y...)		do { \
		if (uaudiodebug) { \
			struct lwp *l = curlwp; \
			printf("%s[%d:%d]: "x, __func__, l->l_proc->p_pid, l->l_lid, y); \
		} \
	} while (0)
#define DPRINTFN_CLEAN(n,x...)	do { \
		if (uaudiodebug > (n)) \
			printf(x); \
	} while (0)
#define DPRINTFN(n,x,y...)	do { \
		if (uaudiodebug > (n)) { \
			struct lwp *l = curlwp; \
			printf("%s[%d:%d]: "x, __func__, l->l_proc->p_pid, l->l_lid, y); \
		} \
	} while (0)
int	uaudiodebug = 0;
#else
#define DPRINTF(x,y...)
#define DPRINTFN_CLEAN(n,x...)
#define DPRINTFN(n,x,y...)
#endif

/* number of outstanding requests */
#define UAUDIO_NCHANBUFS	6
/* number of USB frames per request, also the number of ms */
#define UAUDIO_NFRAMES		10
/* number of microframes per requewst (high, super)  */
#define UAUDIO_NFRAMES_HI	40


#define MIX_MAX_CHAN 8
struct range {
	int minval, maxval, resval;
};

struct mixerctl {
	uint16_t	wValue[MIX_MAX_CHAN]; /* using nchan */
	uint16_t	wIndex;
	uint8_t		nchan;
	uint8_t		type;
#define MIX_ON_OFF	0x01
#define MIX_SELECTOR	0x02
#define MIX_SIGNED_8	0x10
#define MIX_UNSIGNED_8	0x18
#define MIX_SIGNED_16	0x20
#define MIX_UNSIGNED_16	0x28
#define MIX_SIGNED_32	0x40
#define MIX_UNSIGNED_32	0x48
#define MIX_SIZE(n) ( \
	((n) == MIX_UNSIGNED_32 || (n) == MIX_SIGNED_32) ? 4 : \
	((n) == MIX_SIGNED_16 || (n) == MIX_UNSIGNED_16) ? 2 : 1 )
#define MIX_UNSIGNED(n) ( \
	(n) == MIX_UNSIGNED_8 || \
	(n) == MIX_UNSIGNED_16 || \
	(n) == MIX_UNSIGNED_32 )
	struct range	range0;
	struct range	*ranges;
	u_int		nranges;
	u_int		delta;
	u_int		mul;
	uint8_t		class;
	char		ctlname[MAX_AUDIO_DEV_LEN];
	const char	*ctlunit;
};
#define MAKE(h,l) (((h) << 8) | (l))

struct as_info {
	uint8_t		alt;
	uint8_t		encoding;
	uint8_t		nchan;
	uint8_t		attributes; /* Copy of bmAttributes of
				     * usb_audio_streaming_endpoint_descriptor
				     */
	uint8_t		terminal;	/* connected Terminal ID */
	struct usbd_interface *	ifaceh;
	const usb_interface_descriptor_t *idesc;
	const usb_endpoint_descriptor_audio_t *edesc;
	const usb_endpoint_descriptor_audio_t *edesc1;
	const union usb_audio_streaming_type1_descriptor *asf1desc;
	struct audio_format *aformat;
	int		sc_busy;	/* currently used */
};

struct chan {
	void	(*intr)(void *);	/* DMA completion intr handler */
	void	*arg;		/* arg for intr() */
	struct usbd_pipe *pipe;
	struct usbd_pipe *sync_pipe;

	u_int	sample_size;
	u_int	sample_rate;
	u_int	bytes_per_frame;
	u_int	fraction;	/* fraction/1000 is the extra samples/frame */
	u_int	residue;	/* accumulates the fractional samples */

	u_char	*start;		/* upper layer buffer start */
	u_char	*end;		/* upper layer buffer end */
	u_char	*cur;		/* current position in upper layer buffer */
	int	blksize;	/* chunk size to report up */
	int	transferred;	/* transferred bytes not reported up */

	int	altidx;		/* currently used altidx */

	int	curchanbuf;
	u_int	nframes;	/* UAUDIO_NFRAMES or UAUDIO_NFRAMES_HI */
	u_int	nchanbufs;	/* 1..UAUDIO_NCHANBUFS */
	struct chanbuf {
		struct chan	*chan;
		struct usbd_xfer *xfer;
		u_char		*buffer;
		uint16_t	sizes[UAUDIO_NFRAMES_HI];
		uint16_t	offsets[UAUDIO_NFRAMES_HI];
		uint16_t	size;
	} chanbufs[UAUDIO_NCHANBUFS];

	struct uaudio_softc *sc; /* our softc */
};

/*
 *    The MI USB audio subsystem is now MP-SAFE and expects sc_intr_lock to be
 *    held on entry the callbacks passed to uaudio_trigger_{in,out}put
 */
struct uaudio_softc {
	device_t	sc_dev;		/* base device */
	kmutex_t	sc_lock;
	kmutex_t	sc_intr_lock;
	struct usbd_device *sc_udev;	/* USB device */
	int		sc_version;
	int		sc_ac_iface;	/* Audio Control interface */
	struct usbd_interface *	sc_ac_ifaceh;
	struct chan	sc_playchan;	/* play channel */
	struct chan	sc_recchan;	/* record channel */
	int		sc_nullalt;
	int		sc_audio_rev;
	struct as_info	*sc_alts;	/* alternate settings */
	int		sc_nalts;	/* # of alternate settings */
	int		sc_altflags;
#define HAS_8		0x01
#define HAS_16		0x02
#define HAS_8U		0x04
#define HAS_ALAW	0x08
#define HAS_MULAW	0x10
#define UA_NOFRAC	0x20		/* don't do sample rate adjustment */
#define HAS_24		0x40
#define HAS_32		0x80
	int		sc_mode;	/* play/record capability */
	struct mixerctl *sc_ctls;	/* mixer controls */
	int		sc_nctls;	/* # of mixer controls */
	device_t	sc_audiodev;
	int		sc_nratectls;	/* V2 sample rates */
	int		sc_ratectls[AUFMT_MAX_FREQUENCIES];
	int		sc_ratemode[AUFMT_MAX_FREQUENCIES];
	int		sc_playclock;
	int		sc_recclock;
	struct audio_format *sc_formats;
	int		sc_nformats;
	uint8_t		sc_clock[256];	/* map terminals to clocks */
	u_int		sc_channel_config;
	u_int		sc_usb_frames_per_second;
	char		sc_dying;
	struct audio_device sc_adev;
};

struct terminal_list {
	int size;
	uint16_t terminals[1];
};
#define TERMINAL_LIST_SIZE(N)	(offsetof(struct terminal_list, terminals) \
				+ sizeof(uint16_t) * (N))

struct io_terminal {
	union {
		const uaudio_cs_descriptor_t *desc;
		const union usb_audio_input_terminal *it;
		const union usb_audio_output_terminal *ot;
		const struct usb_audio_mixer_unit *mu;
		const struct usb_audio_selector_unit *su;
		const union usb_audio_feature_unit *fu;
		const struct usb_audio_processing_unit *pu;
		const struct usb_audio_extension_unit *eu;
		const struct usb_audio_clksrc_unit *cu;
		const struct usb_audio_clksel_unit *lu;
	} d;
	int inputs_size;
	struct terminal_list **inputs; /* list of source input terminals */
	struct terminal_list *output; /* list of destination output terminals */
	int direct;		/* directly connected to an output terminal */
	uint8_t clock;
};

#define UAC_OUTPUT	0
#define UAC_INPUT	1
#define UAC_EQUAL	2
#define UAC_RECORD	3
#define UAC_NCLASSES	4
#ifdef UAUDIO_DEBUG
Static const char *uac_names[] = {
	AudioCoutputs, AudioCinputs, AudioCequalization, AudioCrecord
};
#endif

#ifdef UAUDIO_DEBUG
Static void uaudio_dump_tml
	(struct terminal_list *tml);
#endif
Static usbd_status uaudio_identify_ac
	(struct uaudio_softc *, const usb_config_descriptor_t *);
Static usbd_status uaudio_identify_as
	(struct uaudio_softc *, const usb_config_descriptor_t *);
Static usbd_status uaudio_process_as
	(struct uaudio_softc *, const char *, int *, int,
	 const usb_interface_descriptor_t *);

Static void	uaudio_add_alt(struct uaudio_softc *, const struct as_info *);

Static const usb_interface_descriptor_t *uaudio_find_iface
	(const char *, int, int *, int);

Static void	uaudio_mixer_add_ctl(struct uaudio_softc *, struct mixerctl *);
Static char	*uaudio_id_name
	(struct uaudio_softc *, const struct io_terminal *, uint8_t);
#ifdef UAUDIO_DEBUG
Static void	uaudio_dump_cluster
	(struct uaudio_softc *, const union usb_audio_cluster *);
#endif
Static union usb_audio_cluster uaudio_get_cluster
	(struct uaudio_softc *, int, const struct io_terminal *);
Static void	uaudio_add_input
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_output
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_mixer
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_selector
	(struct uaudio_softc *, const struct io_terminal *, int);
#ifdef UAUDIO_DEBUG
Static const char *uaudio_get_terminal_name(int);
#endif
Static int	uaudio_determine_class
	(const struct io_terminal *, struct mixerctl *);
Static const char *uaudio_feature_name
	(const struct io_terminal *, uint8_t, int);
Static void	uaudio_add_feature
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_processing_updown
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_processing
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_effect
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_extension
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_clksrc
	(struct uaudio_softc *, const struct io_terminal *, int);
Static void	uaudio_add_clksel
	(struct uaudio_softc *, const struct io_terminal *, int);
Static struct terminal_list *uaudio_merge_terminal_list
	(const struct io_terminal *);
Static struct terminal_list *uaudio_io_terminaltype
	(struct uaudio_softc *, int, struct io_terminal *, int);
Static usbd_status uaudio_identify
	(struct uaudio_softc *, const usb_config_descriptor_t *);
Static u_int uaudio_get_rates
	(struct uaudio_softc *, int, u_int *, u_int);
Static void uaudio_build_formats
	(struct uaudio_softc *);

Static int	uaudio_signext(int, int);
Static int	uaudio_value2bsd(struct mixerctl *, int);
Static int	uaudio_bsd2value(struct mixerctl *, int);
Static const char *uaudio_clockname(u_int);
Static int	uaudio_makename
	(struct uaudio_softc *, uByte, const char *, uByte, char *, size_t);
Static int	uaudio_get(struct uaudio_softc *, int, int, int, int, int);
Static int	uaudio_getbuf(struct uaudio_softc *, int, int, int, int, int, uint8_t *);
Static int	uaudio_ctl_get
	(struct uaudio_softc *, int, struct mixerctl *, int);
Static void	uaudio_set
	(struct uaudio_softc *, int, int, int, int, int, int);
Static void	uaudio_ctl_set
	(struct uaudio_softc *, int, struct mixerctl *, int, int);

Static usbd_status uaudio_speed(struct uaudio_softc *, int, int, uint8_t *, int);
Static usbd_status uaudio_set_speed(struct uaudio_softc *, int, int, u_int);

Static usbd_status uaudio_chan_open(struct uaudio_softc *, struct chan *);
Static void	uaudio_chan_abort(struct uaudio_softc *, struct chan *);
Static void	uaudio_chan_close(struct uaudio_softc *, struct chan *);
Static usbd_status uaudio_chan_alloc_buffers
	(struct uaudio_softc *, struct chan *);
Static void	uaudio_chan_free_buffers(struct uaudio_softc *, struct chan *);
Static void	uaudio_chan_init
	(struct chan *, int, const struct audio_params *, int, bool);
Static void	uaudio_chan_set_param(struct chan *, u_char *, u_char *, int);
Static void	uaudio_chan_ptransfer(struct chan *);
Static void	uaudio_chan_pintr
	(struct usbd_xfer *, void *, usbd_status);

Static void	uaudio_chan_rtransfer(struct chan *);
Static void	uaudio_chan_rintr
	(struct usbd_xfer *, void *, usbd_status);

Static int	uaudio_open(void *, int);
Static int	uaudio_query_format(void *, audio_format_query_t *);
Static int	uaudio_set_format
     (void *, int, const audio_params_t *, const audio_params_t *,
	 audio_filter_reg_t *, audio_filter_reg_t *);
Static int	uaudio_round_blocksize(void *, int, int, const audio_params_t *);
Static int	uaudio_trigger_output
	(void *, void *, void *, int, void (*)(void *), void *,
	 const audio_params_t *);
Static int	uaudio_trigger_input
	(void *, void *, void *, int, void (*)(void *), void *,
	 const audio_params_t *);
Static int	uaudio_halt_in_dma(void *);
Static int	uaudio_halt_out_dma(void *);
Static void	uaudio_halt_in_dma_unlocked(struct uaudio_softc *);
Static void	uaudio_halt_out_dma_unlocked(struct uaudio_softc *);
Static int	uaudio_getdev(void *, struct audio_device *);
Static int	uaudio_mixer_set_port(void *, mixer_ctrl_t *);
Static int	uaudio_mixer_get_port(void *, mixer_ctrl_t *);
Static int	uaudio_query_devinfo(void *, mixer_devinfo_t *);
Static int	uaudio_get_props(void *);
Static void	uaudio_get_locks(void *, kmutex_t **, kmutex_t **);

Static const struct audio_hw_if uaudio_hw_if = {
	.open			= uaudio_open,
	.query_format		= uaudio_query_format,
	.set_format		= uaudio_set_format,
	.round_blocksize	= uaudio_round_blocksize,
	.halt_output		= uaudio_halt_out_dma,
	.halt_input		= uaudio_halt_in_dma,
	.getdev			= uaudio_getdev,
	.set_port		= uaudio_mixer_set_port,
	.get_port		= uaudio_mixer_get_port,
	.query_devinfo		= uaudio_query_devinfo,
	.get_props		= uaudio_get_props,
	.trigger_output		= uaudio_trigger_output,
	.trigger_input		= uaudio_trigger_input,
	.get_locks		= uaudio_get_locks,
};

static int uaudio_match(device_t, cfdata_t, void *);
static void uaudio_attach(device_t, device_t, void *);
static int uaudio_detach(device_t, int);
static void uaudio_childdet(device_t, device_t);
static int uaudio_activate(device_t, enum devact);


CFATTACH_DECL2_NEW(uaudio, sizeof(struct uaudio_softc),
    uaudio_match, uaudio_attach, uaudio_detach, uaudio_activate, NULL,
    uaudio_childdet);

static int
uaudio_match(device_t parent, cfdata_t match, void *aux)
{
	struct usbif_attach_arg *uiaa = aux;

	/* Trigger on the control interface. */
	if (uiaa->uiaa_class != UICLASS_AUDIO ||
	    uiaa->uiaa_subclass != UISUBCLASS_AUDIOCONTROL ||
	    (usbd_get_quirks(uiaa->uiaa_device)->uq_flags & UQ_BAD_AUDIO))
		return UMATCH_NONE;

	return UMATCH_IFACECLASS_IFACESUBCLASS;
}

static void
uaudio_attach(device_t parent, device_t self, void *aux)
{
	struct uaudio_softc *sc = device_private(self);
	struct usbif_attach_arg *uiaa = aux;
	usb_interface_descriptor_t *id;
	usb_config_descriptor_t *cdesc;
	char *devinfop;
	usbd_status err;
	int i, j, found;

	sc->sc_dev = self;
	sc->sc_udev = uiaa->uiaa_device;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SOFTUSB);

	strlcpy(sc->sc_adev.name, "USB audio", sizeof(sc->sc_adev.name));
	strlcpy(sc->sc_adev.version, "", sizeof(sc->sc_adev.version));
	snprintf(sc->sc_adev.config, sizeof(sc->sc_adev.config), "usb:%08x",
	    sc->sc_udev->ud_cookie.cookie);

	aprint_naive("\n");
	aprint_normal("\n");

	devinfop = usbd_devinfo_alloc(uiaa->uiaa_device, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	cdesc = usbd_get_config_descriptor(sc->sc_udev);
	if (cdesc == NULL) {
		aprint_error_dev(self,
		    "failed to get configuration descriptor\n");
		return;
	}

	err = uaudio_identify(sc, cdesc);
	if (err) {
		aprint_error_dev(self,
		    "audio descriptors make no sense, error=%d\n", err);
		return;
	}

	sc->sc_ac_ifaceh = uiaa->uiaa_iface;
	/* Pick up the AS interface. */
	for (i = 0; i < uiaa->uiaa_nifaces; i++) {
		if (uiaa->uiaa_ifaces[i] == NULL)
			continue;
		id = usbd_get_interface_descriptor(uiaa->uiaa_ifaces[i]);
		if (id == NULL)
			continue;
		found = 0;
		for (j = 0; j < sc->sc_nalts; j++) {
			if (id->bInterfaceNumber ==
			    sc->sc_alts[j].idesc->bInterfaceNumber) {
				sc->sc_alts[j].ifaceh = uiaa->uiaa_ifaces[i];
				found = 1;
			}
		}
		if (found)
			uiaa->uiaa_ifaces[i] = NULL;
	}

	for (j = 0; j < sc->sc_nalts; j++) {
		if (sc->sc_alts[j].ifaceh == NULL) {
			aprint_error_dev(self,
			    "alt %d missing AS interface(s)\n", j);
			return;
		}
	}

	aprint_normal_dev(self, "audio rev %d.%02x\n",
	       sc->sc_audio_rev >> 8, sc->sc_audio_rev & 0xff);

	sc->sc_playchan.sc = sc->sc_recchan.sc = sc;
	sc->sc_playchan.altidx = -1;
	sc->sc_recchan.altidx = -1;

	switch (sc->sc_udev->ud_speed) {
	case USB_SPEED_LOW:
	case USB_SPEED_FULL:
		sc->sc_usb_frames_per_second = USB_FRAMES_PER_SECOND;
		sc->sc_playchan.nframes =
		    sc->sc_recchan.nframes = UAUDIO_NFRAMES;
		break;
	default: /* HIGH, SUPER, SUPER_PLUS, more ? */
		sc->sc_usb_frames_per_second = USB_FRAMES_PER_SECOND * USB_UFRAMES_PER_FRAME;
		sc->sc_playchan.nframes =
		    sc->sc_recchan.nframes = UAUDIO_NFRAMES_HI;
		break;
	}
	sc->sc_playchan.nchanbufs =
	    sc->sc_recchan.nchanbufs = UAUDIO_NCHANBUFS;

	DPRINTF("usb fps %u, max channel frames %u, max channel buffers %u\n",
	    sc->sc_usb_frames_per_second, sc->sc_playchan.nframes, sc->sc_playchan.nchanbufs);

	if (usbd_get_quirks(sc->sc_udev)->uq_flags & UQ_AU_NO_FRAC)
		sc->sc_altflags |= UA_NOFRAC;

#ifndef UAUDIO_DEBUG
	if (bootverbose)
#endif
		aprint_normal_dev(self, "%d mixer controls\n",
		    sc->sc_nctls);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);

	DPRINTF("%s", "doing audio_attach_mi\n");
	sc->sc_audiodev = audio_attach_mi(&uaudio_hw_if, sc, sc->sc_dev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;
}

static int
uaudio_activate(device_t self, enum devact act)
{
	struct uaudio_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static void
uaudio_childdet(device_t self, device_t child)
{
	struct uaudio_softc *sc = device_private(self);

	KASSERT(sc->sc_audiodev == child);
	sc->sc_audiodev = NULL;
}

static int
uaudio_detach(device_t self, int flags)
{
	struct uaudio_softc *sc = device_private(self);
	int rv, i;

	sc->sc_dying = 1;

	pmf_device_deregister(self);

	/* Wait for outstanding requests to complete. */
	uaudio_halt_out_dma_unlocked(sc);
	uaudio_halt_in_dma_unlocked(sc);

	if (sc->sc_audiodev != NULL) {
		rv = config_detach(sc->sc_audiodev, flags);
		if (rv)
			return rv;
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev, sc->sc_dev);

	if (sc->sc_formats != NULL)
		kmem_free(sc->sc_formats,
		    sizeof(struct audio_format) * sc->sc_nformats);

	if (sc->sc_ctls != NULL) {
		for (i=0; i<sc->sc_nctls; ++i) {
			if (sc->sc_ctls[i].nranges == 0)
				continue;
			kmem_free( sc->sc_ctls[i].ranges,
			    sc->sc_ctls[i].nranges * sizeof(struct range));
		}
		kmem_free(sc->sc_ctls, sizeof(struct mixerctl) * sc->sc_nctls);
	}

	if (sc->sc_alts != NULL)
		kmem_free(sc->sc_alts, sizeof(struct as_info) * sc->sc_nalts);

	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);

	return 0;
}

Static int
uaudio_query_format(void *addr, audio_format_query_t *afp)
{
	struct uaudio_softc *sc;

	sc = addr;
	return audio_query_format(sc->sc_formats, sc->sc_nformats, afp);
}

Static const usb_interface_descriptor_t *
uaudio_find_iface(const char *tbuf, int size, int *offsp, int subtype)
{
	const usb_interface_descriptor_t *d;

	while (*offsp + sizeof(*d) <= size) {
		d = (const void *)(tbuf + *offsp);
		DPRINTFN(3, "%d + %d <= %d type %d class %d/%d iface %d\n",
		    *offsp, d->bLength, size,
		    d->bDescriptorType,
		    d->bInterfaceClass,
		    d->bInterfaceSubClass,
		    d->bInterfaceNumber);
		*offsp += d->bLength;
		if (d->bDescriptorType == UDESC_INTERFACE &&
		    d->bInterfaceClass == UICLASS_AUDIO &&
		    d->bInterfaceSubClass == subtype)
			return d;
	}
	return NULL;
}

Static void
uaudio_mixer_add_ctl(struct uaudio_softc *sc, struct mixerctl *mc)
{
	int res;
	size_t len, count, msz;
	struct mixerctl *nmc;
	struct range *r;
	uint8_t *buf, *p;
	int i;

	if (mc->class < UAC_NCLASSES) {
		DPRINTF("adding %s.%s\n", uac_names[mc->class], mc->ctlname);
	} else {
		DPRINTF("adding %s\n", mc->ctlname);
	}
	len = sizeof(*mc) * (sc->sc_nctls + 1);
	nmc = kmem_alloc(len, KM_SLEEP);
	/* Copy old data, if there was any */
	if (sc->sc_nctls != 0) {
		memcpy(nmc, sc->sc_ctls, sizeof(*mc) * sc->sc_nctls);
		for (i = 0; i<sc->sc_nctls; ++i) {
			if (sc->sc_ctls[i].ranges == &sc->sc_ctls[i].range0)
				nmc[i].ranges = &nmc[i].range0;
		}
		kmem_free(sc->sc_ctls, sizeof(*mc) * sc->sc_nctls);
	}
	sc->sc_ctls = nmc;

	/*
	 * preset
	 * - mc->class
	 * - mc->ctlname
	 * - mc->ctlunit
	 * - mc->wIndex
	 * - mc->wValue[]
	 * - mc->type
	 * - mc->nchan
	 *
	 * - mc->range0, mc->mul for MIX_SELECTOR
	 */
	sc->sc_ctls[sc->sc_nctls] = *mc;
	mc = &sc->sc_ctls[sc->sc_nctls++];
	msz = MIX_SIZE(mc->type);

	mc->delta = 0;
	mc->nranges = 0;
	mc->ranges = r = &mc->range0;
	mc->mul = 0;
	if (mc->type == MIX_ON_OFF) {
		r->minval = 0;
		r->maxval = 1;
		r->resval = 1;
		res = r->resval;
	} else if (mc->type == MIX_SELECTOR) {
		/* range0 already set by uaudio_add_selector */
		res = r->resval;
	} else if (sc->sc_version == UAUDIO_VERSION1) {
		/* Determine min and max values. */
		r->minval = uaudio_signext(mc->type,
			uaudio_get(sc, GET_MIN, UT_READ_CLASS_INTERFACE,
				   mc->wValue[0], mc->wIndex, msz));
		r->maxval = uaudio_signext(mc->type,
			uaudio_get(sc, GET_MAX, UT_READ_CLASS_INTERFACE,
				   mc->wValue[0], mc->wIndex, msz));
		r->resval = uaudio_get(sc, GET_RES, UT_READ_CLASS_INTERFACE,
			 mc->wValue[0], mc->wIndex, msz);
		mc->mul = r->maxval - r->minval;
		res = r->resval;
	} else { /* UAUDIO_VERSION2 */
		count = (uint16_t)uaudio_get(sc, V2_RANGES,
		    UT_READ_CLASS_INTERFACE,
		    mc->wValue[0], mc->wIndex, 2);

		if (count == 0 || count == (uint16_t)-1) {
			DPRINTF("invalid range count %zu\n", count);
			return;
		}

		if (count > 1) {
			r = kmem_alloc(sizeof(struct range) * count,
			    KM_SLEEP);
			mc->ranges = r;
			mc->nranges = count;
		}

		mc->ranges[0].minval = 0;
		mc->ranges[0].maxval = 0;
		mc->ranges[0].resval = 1;

		/* again with the required buffer size */
		len = 2 + count * 3 * msz;
		buf = kmem_alloc(len, KM_SLEEP);
		uaudio_getbuf(sc, V2_RANGES, UT_READ_CLASS_INTERFACE,
				 mc->wValue[0], mc->wIndex, len, buf);
		res = 0;
		p = &buf[2];
		for (i=0, p=buf+2; i<count; ++i) {
			uint32_t minval, maxval, resval;
			switch (msz) {
			case 1:
				minval = *p++;
				maxval = *p++;
				resval = *p++;
				break;
			case 2:
				minval = p[0] | p[1] << 8;
				p += 2;
				maxval = p[0] | p[1] << 8;
				p += 2;
				resval = p[0] | p[1] << 8;
				p += 2;
				break;
			case 3:
				minval = p[0] | p[1] << 8 | p[2] << 16;
				p += 3;
				maxval = p[0] | p[1] << 8 | p[2] << 16;
				p += 3;
				resval = p[0] | p[1] << 8 | p[2] << 16;
				p += 3;
				break;
			case 4:
				minval = p[0] | p[1] << 8 \
				       | p[2] << 16 | p[3] << 24;
				p += 4;
				maxval = p[0] | p[1] << 8 \
				       | p[2] << 16 | p[3] << 24;
				p += 4;
				resval = p[0] | p[1] << 8 \
				       | p[2] << 16 | p[3] << 24;
				p += 4;
				break;
			default: /* not allowed */
				minval = maxval = 0;
				resval = 1;
				break;
			}
			mc->ranges[i].minval = uaudio_signext(mc->type, minval);
			mc->ranges[i].maxval = uaudio_signext(mc->type, maxval);
			mc->ranges[i].resval = uaudio_signext(mc->type, resval);
			if (mc->ranges[i].resval > res)
				res = mc->ranges[i].resval;
		}
		kmem_free(buf, len);
		
		mc->mul = mc->ranges[count - 1].maxval - mc->ranges[0].minval;

		/*
		 * use resolution 1 (ideally the lcd) for
		 * multiple (valid) resolution values.
		 */
		if (count > 1 && res > 0)
			res = 1;
	}

	if (mc->mul == 0)
		mc->mul = 1;

	mc->delta = (res * 255 + mc->mul - 1) / mc->mul;

#ifdef UAUDIO_DEBUG
	if (uaudiodebug > 2) {
		DPRINTFN_CLEAN(2, "wValue=%04x", mc->wValue[0]);
		for (i = 1; i < mc->nchan; i++)
			DPRINTFN_CLEAN(2, ",%04x", mc->wValue[i]);
		DPRINTFN_CLEAN(2, "\n");
		count = mc->nranges > 0 ? mc->nranges : 1;
		for (i = 0; i < count; i++)
			DPRINTFN_CLEAN(2, "%d: wIndex=%04x type=%d name='%s' "
			 "unit='%s' min=%d max=%d res=%d\n",
			 i, mc->wIndex, mc->type, mc->ctlname, mc->ctlunit,
			 mc->ranges[i].minval,
		         mc->ranges[i].maxval,
		         mc->ranges[i].resval);
	}
#endif
}

Static char *
uaudio_id_name(struct uaudio_softc *sc,
    const struct io_terminal *iot, uint8_t id)
{
	static char tbuf[32];

	snprintf(tbuf, sizeof(tbuf), "i%u", id);

	return tbuf;
}

#ifdef UAUDIO_DEBUG
Static void
uaudio_dump_cluster(struct uaudio_softc *sc, const union usb_audio_cluster *cl)
{
	static const char *channel_v1_names[16] = {
		"LEFT", "RIGHT", "CENTER", "LFE",
		"LEFT_SURROUND", "RIGHT_SURROUND", "LEFT_CENTER", "RIGHT_CENTER",
		"SURROUND", "LEFT_SIDE", "RIGHT_SIDE", "TOP",
		"RESERVED12", "RESERVED13", "RESERVED14", "RESERVED15",
	};
	static const char *channel_v2_names[32] = {
		"LEFT", "RIGHT", "CENTER", "LFE",
		"BACK_LEFT", "BACK_RIGHT", "FLC", "FRC",
		"BACK_CENTER", "SIDE_LEFT", "SIDE_RIGHT", "TOP CENTER",
		"TFL", "TFC", "TFR", "TBL", "TBC", "TBR",
		"TFLC", "TFRC", "LLFE", "RLFE", "TSL", "TSR",
		"BC", "BLC", "BRC",
		"RESERVED27", "RESERVED28", "RESERVED29", "RESERVED30",
		"RAW_DATA"
	};
	const char **channel_names;
	uint32_t cc;
	int i, first, icn;

	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
		channel_names = channel_v1_names;
		cc = UGETW(cl->v1.wChannelConfig);
		icn = cl->v1.iChannelNames;
		printf("cluster: bNrChannels=%u wChannelConfig=%#.4x",
			  cl->v1.bNrChannels, cc);
		break;
	case UAUDIO_VERSION2:
		channel_names = channel_v2_names;
		cc = UGETDW(cl->v2.bmChannelConfig);
		icn = cl->v2.iChannelNames;
		printf("cluster: bNrChannels=%u bmChannelConfig=%#.8x",
			  cl->v2.bNrChannels, cc);
		break;
	default:
		return;
	}

	first = TRUE;
	for (i = 0; cc != 0; i++) {
		if (cc & 1) {
			printf("%c%s", first ? '<' : ',', channel_names[i]);
			first = FALSE;
		}
		cc = cc >> 1;
	}
	printf("> iChannelNames=%u", icn);
}
#endif

Static union usb_audio_cluster
uaudio_get_cluster(struct uaudio_softc *sc, int id, const struct io_terminal *iot)
{
	union usb_audio_cluster r;
	const uaudio_cs_descriptor_t *dp;
	u_int pins;
	int i;

	for (i = 0; i < 25; i++) { /* avoid infinite loops */
		dp = iot[id].d.desc;
		if (dp == 0)
			goto bad;

		switch (dp->bDescriptorSubtype) {
		case UDESCSUB_AC_INPUT:
			switch (sc->sc_version) {
			case UAUDIO_VERSION1:
				r.v1.bNrChannels = iot[id].d.it->v1.bNrChannels;
				USETW(r.v1.wChannelConfig,
				    UGETW(iot[id].d.it->v1.wChannelConfig));
				r.v1.iChannelNames = iot[id].d.it->v1.iChannelNames;
				break;
			case UAUDIO_VERSION2:
				r.v2.bNrChannels = iot[id].d.it->v2.bNrChannels;
				USETDW(r.v2.bmChannelConfig,
				    UGETW(iot[id].d.it->v2.bmChannelConfig));
				r.v2.iChannelNames = iot[id].d.it->v2.iChannelNames;
				break;
			}
			return r;
		case UDESCSUB_AC_OUTPUT:
			/* XXX This is not really right */
			id = iot[id].d.ot->v1.bSourceId;
			break;
		case UDESCSUB_AC_MIXER:
			switch (sc->sc_version) {
			case UAUDIO_VERSION1:
				pins = iot[id].d.mu->bNrInPins;
				r.v1 = *(const struct usb_audio_v1_cluster *)
				    &iot[id].d.mu->baSourceId[pins];
				break;
			case UAUDIO_VERSION2:
				pins = iot[id].d.mu->bNrInPins;
				r.v2 = *(const struct usb_audio_v2_cluster *)
				    &iot[id].d.mu->baSourceId[pins];
				break;
			}
			return r;
		case UDESCSUB_AC_SELECTOR:
			/* XXX This is not really right */
			id = iot[id].d.su->baSourceId[0];
			break;
		case UDESCSUB_AC_FEATURE:
			/* XXX This is not really right */
			switch (sc->sc_version) {
			case UAUDIO_VERSION1:
				id = iot[id].d.fu->v1.bSourceId;
				break;
			case UAUDIO_VERSION2:
				id = iot[id].d.fu->v2.bSourceId;
				break;
			}
			break;
		case UDESCSUB_AC_PROCESSING:
			switch (sc->sc_version) {
			case UAUDIO_VERSION1:
				pins = iot[id].d.pu->bNrInPins;
				r.v1 = *(const struct usb_audio_v1_cluster *)
				    &iot[id].d.pu->baSourceId[pins];
				break;
			case UAUDIO_VERSION2:
				pins = iot[id].d.pu->bNrInPins;
				r.v2 = *(const struct usb_audio_v2_cluster *)
				    &iot[id].d.pu->baSourceId[pins];
				break;
			}
			return r;
		case UDESCSUB_AC_EXTENSION:
			switch (sc->sc_version) {
			case UAUDIO_VERSION1:
				pins = iot[id].d.eu->bNrInPins;
				r.v1 = *(const struct usb_audio_v1_cluster *)
				    &iot[id].d.eu->baSourceId[pins];
				break;
			case UAUDIO_VERSION2:
				pins = iot[id].d.eu->bNrInPins;
				r.v2 = *(const struct usb_audio_v2_cluster *)
				    &iot[id].d.eu->baSourceId[pins];
				break;
			}
			return r;
		default:
			goto bad;
		}
	}
 bad:
	aprint_error("uaudio_get_cluster: bad data\n");
	memset(&r, 0, sizeof(r));
	return r;

}

Static void
uaudio_add_input(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{
	const union usb_audio_input_terminal *d;

	d = iot[id].d.it;
	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
#ifdef UAUDIO_DEBUG
		DPRINTFN(2,"bTerminalId=%d wTerminalType=0x%04x "
			    "bAssocTerminal=%d bNrChannels=%d wChannelConfig=%d "
			    "iChannelNames=%d iTerminal=%d\n",
			    d->v1.bTerminalId, UGETW(d->v1.wTerminalType), d->v1.bAssocTerminal,
			    d->v1.bNrChannels, UGETW(d->v1.wChannelConfig),
			    d->v1.iChannelNames, d->v1.iTerminal);
#endif
		/* If USB input terminal, record wChannelConfig */
		if ((UGETW(d->v1.wTerminalType) & 0xff00) != UAT_UNDEFINED)
			return;
		sc->sc_channel_config = UGETW(d->v1.wChannelConfig);
		sc->sc_clock[id] = 0;
		break;
	case UAUDIO_VERSION2:
#ifdef UAUDIO_DEBUG
		DPRINTFN(2,"bTerminalId=%d wTerminalType=0x%04x "
			    "bAssocTerminal=%d bNrChannels=%d bmChannelConfig=%x "
			    "iChannelNames=%d bCSourceId=%d iTerminal=%d\n",
			    d->v2.bTerminalId, UGETW(d->v2.wTerminalType), d->v2.bAssocTerminal,
			    d->v2.bNrChannels, UGETDW(d->v2.bmChannelConfig),
			    d->v2.iChannelNames, d->v2.bCSourceId, d->v2.iTerminal);
#endif
		/* If USB input terminal, record wChannelConfig */
		if ((UGETW(d->v2.wTerminalType) & 0xff00) != UAT_UNDEFINED)
			return;
		sc->sc_channel_config = UGETDW(d->v2.bmChannelConfig);
		sc->sc_clock[id] = d->v2.bCSourceId;
		break;
	}
}

Static void
uaudio_add_output(struct uaudio_softc *sc,
    const struct io_terminal *iot, int id)
{
#ifdef UAUDIO_DEBUG
	const union usb_audio_output_terminal *d;

	d = iot[id].d.ot;
	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
		DPRINTFN(2,"bTerminalId=%d wTerminalType=0x%04x "
			    "bAssocTerminal=%d bSourceId=%d iTerminal=%d\n",
			    d->v1.bTerminalId, UGETW(d->v1.wTerminalType), d->v1.bAssocTerminal,
			    d->v1.bSourceId, d->v1.iTerminal);
		sc->sc_clock[id] = 0;
		break;
	case UAUDIO_VERSION2:
		DPRINTFN(2,"bTerminalId=%d wTerminalType=0x%04x "
			    "bAssocTerminal=%d bSourceId=%d bCSourceId=%d, iTerminal=%d\n",
			    d->v2.bTerminalId, UGETW(d->v2.wTerminalType), d->v2.bAssocTerminal,
			    d->v2.bSourceId, d->v2.bCSourceId, d->v2.iTerminal);
		sc->sc_clock[id] = d->v2.bCSourceId;
		break;
	}
#endif
}

Static void
uaudio_add_mixer(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{
	const struct usb_audio_mixer_unit *d;
	const union usb_audio_mixer_unit_1 *d1;
	int c, chs, ichs, ochs, nchs, i, o, bno, p, k;
	size_t bm_size;
	const uByte *bm;
	struct mixerctl mix;

	d = iot[id].d.mu;
	d1 = (const union usb_audio_mixer_unit_1 *)&d->baSourceId[d->bNrInPins];
	DPRINTFN(2,"bUnitId=%d bNrInPins=%d\n",
		    d->bUnitId, d->bNrInPins);

	mix.wIndex = MAKE(d->bUnitId, sc->sc_ac_iface);
	uaudio_determine_class(&iot[id], &mix);
	mix.type = MIX_SIGNED_16;
	mix.ctlunit = AudioNvolume;

	/* Compute the number of input channels */
	/* and the number of output channels */
	ichs = 0;
	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
		for (i = 0; i < d->bNrInPins; i++)
			ichs += uaudio_get_cluster(sc, d->baSourceId[i], iot).v1.bNrChannels;
		ochs = d1->v1.bNrChannels;
		DPRINTFN(2,"ichs=%d ochs=%d\n", ichs, ochs);
		bm = d1->v1.bmControls;
		break;
	case UAUDIO_VERSION2:
		for (i = 0; i < d->bNrInPins; i++)
			ichs += uaudio_get_cluster(sc, d->baSourceId[i], iot).v2.bNrChannels;
		ochs = d1->v2.bNrChannels;
		DPRINTFN(2,"ichs=%d ochs=%d\n", ichs, ochs);
		bm = d1->v2.bmMixerControls;
		bm_size = ichs * ochs / 8 + ((ichs * ochs % 8) ? 1 : 0);
		/* bmControls */
		if ((bm[bm_size] & UA_MIX_CLUSTER_MASK) != UA_MIX_CLUSTER_RW)
			return;
		break;
	default:
		return;
	}

	for (p = i = 0; i < d->bNrInPins; i++) {
		switch (sc->sc_version) {
		case UAUDIO_VERSION1:
			chs = uaudio_get_cluster(sc, d->baSourceId[i], iot)
			    .v1.bNrChannels;
			break;
		case UAUDIO_VERSION2:
			chs = uaudio_get_cluster(sc, d->baSourceId[i], iot)
			    .v2.bNrChannels;
			break;
		default:
			continue;
		}

#define _BIT(bno) ((bm[bno / 8] >> (7 - bno % 8)) & 1)

		nchs = chs < MIX_MAX_CHAN ? chs : MIX_MAX_CHAN;

		k = 0;
		for (c = 0; c < nchs; c++) {
			for (o = 0; o < ochs; o++) {
				bno = (p + c) * ochs + o;
				if (_BIT(bno))
					mix.wValue[k++] =
						MAKE(p+c+1, o+1);
			}
		}
		mix.nchan = nchs;

		snprintf(mix.ctlname, sizeof(mix.ctlname),
		    "mix%d-%s", d->bUnitId,
		    uaudio_id_name(sc, iot, d->baSourceId[i])
		);
		uaudio_mixer_add_ctl(sc, &mix);

#undef _BIT

		p += chs;
	}
}

Static void
uaudio_add_selector(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{
	const struct usb_audio_selector_unit *d;
	struct mixerctl mix;
	int i, wp;

	d = iot[id].d.su;
	DPRINTFN(2,"bUnitId=%d bNrInPins=%d\n",
		    d->bUnitId, d->bNrInPins);
	mix.wIndex = MAKE(d->bUnitId, sc->sc_ac_iface);
	if (sc->sc_version == UAUDIO_VERSION2)
		mix.wValue[0] = MAKE(V2_CUR_SELECTOR, 0);
	else
		mix.wValue[0] = MAKE(0, 0);
	uaudio_determine_class(&iot[id], &mix);
	mix.nchan = 1;
	mix.type = MIX_SELECTOR;
	mix.ctlunit = "";
	mix.range0.minval = 1;
	mix.range0.maxval = d->bNrInPins;
	mix.range0.resval = 1;
	mix.mul = mix.range0.maxval - mix.range0.minval;
	wp = snprintf(mix.ctlname, MAX_AUDIO_DEV_LEN, "sel%d-", d->bUnitId);
	for (i = 1; i <= d->bNrInPins; i++) {
		wp += strlcpy(mix.ctlname + wp,
		    uaudio_id_name(sc, iot, d->baSourceId[i-1]),
		    MAX_AUDIO_DEV_LEN - wp);
		if (wp > MAX_AUDIO_DEV_LEN - 1)
			break;
	}
	uaudio_mixer_add_ctl(sc, &mix);
}

#ifdef UAUDIO_DEBUG
Static const char *
uaudio_get_terminal_name(int terminal_type)
{
	static char tbuf[100];

	switch (terminal_type) {
	/* USB terminal types */
	case UAT_UNDEFINED:	return "UAT_UNDEFINED";
	case UAT_STREAM:	return "UAT_STREAM";
	case UAT_VENDOR:	return "UAT_VENDOR";
	/* input terminal types */
	case UATI_UNDEFINED:	return "UATI_UNDEFINED";
	case UATI_MICROPHONE:	return "UATI_MICROPHONE";
	case UATI_DESKMICROPHONE:	return "UATI_DESKMICROPHONE";
	case UATI_PERSONALMICROPHONE:	return "UATI_PERSONALMICROPHONE";
	case UATI_OMNIMICROPHONE:	return "UATI_OMNIMICROPHONE";
	case UATI_MICROPHONEARRAY:	return "UATI_MICROPHONEARRAY";
	case UATI_PROCMICROPHONEARR:	return "UATI_PROCMICROPHONEARR";
	/* output terminal types */
	case UATO_UNDEFINED:	return "UATO_UNDEFINED";
	case UATO_SPEAKER:	return "UATO_SPEAKER";
	case UATO_HEADPHONES:	return "UATO_HEADPHONES";
	case UATO_DISPLAYAUDIO:	return "UATO_DISPLAYAUDIO";
	case UATO_DESKTOPSPEAKER:	return "UATO_DESKTOPSPEAKER";
	case UATO_ROOMSPEAKER:	return "UATO_ROOMSPEAKER";
	case UATO_COMMSPEAKER:	return "UATO_COMMSPEAKER";
	case UATO_SUBWOOFER:	return "UATO_SUBWOOFER";
	/* bidir terminal types */
	case UATB_UNDEFINED:	return "UATB_UNDEFINED";
	case UATB_HANDSET:	return "UATB_HANDSET";
	case UATB_HEADSET:	return "UATB_HEADSET";
	case UATB_SPEAKERPHONE:	return "UATB_SPEAKERPHONE";
	case UATB_SPEAKERPHONEESUP:	return "UATB_SPEAKERPHONEESUP";
	case UATB_SPEAKERPHONEECANC:	return "UATB_SPEAKERPHONEECANC";
	/* telephony terminal types */
	case UATT_UNDEFINED:	return "UATT_UNDEFINED";
	case UATT_PHONELINE:	return "UATT_PHONELINE";
	case UATT_TELEPHONE:	return "UATT_TELEPHONE";
	case UATT_DOWNLINEPHONE:	return "UATT_DOWNLINEPHONE";
	/* external terminal types */
	case UATE_UNDEFINED:	return "UATE_UNDEFINED";
	case UATE_ANALOGCONN:	return "UATE_ANALOGCONN";
	case UATE_LINECONN:	return "UATE_LINECONN";
	case UATE_LEGACYCONN:	return "UATE_LEGACYCONN";
	case UATE_DIGITALAUIFC:	return "UATE_DIGITALAUIFC";
	case UATE_SPDIF:	return "UATE_SPDIF";
	case UATE_1394DA:	return "UATE_1394DA";
	case UATE_1394DV:	return "UATE_1394DV";
	/* embedded function terminal types */
	case UATF_UNDEFINED:	return "UATF_UNDEFINED";
	case UATF_CALIBNOISE:	return "UATF_CALIBNOISE";
	case UATF_EQUNOISE:	return "UATF_EQUNOISE";
	case UATF_CDPLAYER:	return "UATF_CDPLAYER";
	case UATF_DAT:	return "UATF_DAT";
	case UATF_DCC:	return "UATF_DCC";
	case UATF_MINIDISK:	return "UATF_MINIDISK";
	case UATF_ANALOGTAPE:	return "UATF_ANALOGTAPE";
	case UATF_PHONOGRAPH:	return "UATF_PHONOGRAPH";
	case UATF_VCRAUDIO:	return "UATF_VCRAUDIO";
	case UATF_VIDEODISCAUDIO:	return "UATF_VIDEODISCAUDIO";
	case UATF_DVDAUDIO:	return "UATF_DVDAUDIO";
	case UATF_TVTUNERAUDIO:	return "UATF_TVTUNERAUDIO";
	case UATF_SATELLITE:	return "UATF_SATELLITE";
	case UATF_CABLETUNER:	return "UATF_CABLETUNER";
	case UATF_DSS:	return "UATF_DSS";
	case UATF_RADIORECV:	return "UATF_RADIORECV";
	case UATF_RADIOXMIT:	return "UATF_RADIOXMIT";
	case UATF_MULTITRACK:	return "UATF_MULTITRACK";
	case UATF_SYNTHESIZER:	return "UATF_SYNTHESIZER";
	default:
		snprintf(tbuf, sizeof(tbuf), "unknown type (%#.4x)", terminal_type);
		return tbuf;
	}
}
#endif

Static int
uaudio_determine_class(const struct io_terminal *iot, struct mixerctl *mix)
{
	int terminal_type;

	if (iot == NULL || iot->output == NULL) {
		mix->class = UAC_OUTPUT;
		return 0;
	}
	terminal_type = 0;
	if (iot->output->size == 1)
		terminal_type = iot->output->terminals[0];
	/*
	 * If the only output terminal is USB,
	 * the class is UAC_RECORD.
	 */
	if ((terminal_type & 0xff00) == (UAT_UNDEFINED & 0xff00)) {
		mix->class = UAC_RECORD;
		if (iot->inputs_size == 1
		    && iot->inputs[0] != NULL
		    && iot->inputs[0]->size == 1)
			return iot->inputs[0]->terminals[0];
		else
			return 0;
	}
	/*
	 * If the ultimate destination of the unit is just one output
	 * terminal and the unit is connected to the output terminal
	 * directly, the class is UAC_OUTPUT.
	 */
	if (terminal_type != 0 && iot->direct) {
		mix->class = UAC_OUTPUT;
		return terminal_type;
	}
	/*
	 * If the unit is connected to just one input terminal,
	 * the class is UAC_INPUT.
	 */
	if (iot->inputs_size == 1 && iot->inputs[0] != NULL
	    && iot->inputs[0]->size == 1) {
		mix->class = UAC_INPUT;
		return iot->inputs[0]->terminals[0];
	}
	/*
	 * Otherwise, the class is UAC_OUTPUT.
	 */
	mix->class = UAC_OUTPUT;
	return terminal_type;
}

Static const char *
uaudio_feature_name(const struct io_terminal *iot,
    uint8_t class, int terminal_type)
{

	if (class == UAC_RECORD && terminal_type == 0)
		return AudioNmixerout;

	DPRINTF("terminal_type=%s\n", uaudio_get_terminal_name(terminal_type));
	switch (terminal_type) {
	case UAT_STREAM:
		return AudioNdac;

	case UATI_MICROPHONE:
	case UATI_DESKMICROPHONE:
	case UATI_PERSONALMICROPHONE:
	case UATI_OMNIMICROPHONE:
	case UATI_MICROPHONEARRAY:
	case UATI_PROCMICROPHONEARR:
		return AudioNmicrophone;

	case UATO_SPEAKER:
	case UATO_DESKTOPSPEAKER:
	case UATO_ROOMSPEAKER:
	case UATO_COMMSPEAKER:
		return AudioNspeaker;

	case UATO_HEADPHONES:
		return AudioNheadphone;

	case UATO_SUBWOOFER:
		return AudioNlfe;

	/* telephony terminal types */
	case UATT_UNDEFINED:
	case UATT_PHONELINE:
	case UATT_TELEPHONE:
	case UATT_DOWNLINEPHONE:
		return "phone";

	case UATE_ANALOGCONN:
	case UATE_LINECONN:
	case UATE_LEGACYCONN:
		return AudioNline;

	case UATE_DIGITALAUIFC:
	case UATE_SPDIF:
	case UATE_1394DA:
	case UATE_1394DV:
		return AudioNaux;

	case UATF_CDPLAYER:
		return AudioNcd;

	case UATF_SYNTHESIZER:
		return AudioNfmsynth;

	case UATF_VIDEODISCAUDIO:
	case UATF_DVDAUDIO:
	case UATF_TVTUNERAUDIO:
		return AudioNvideo;

	case UAT_UNDEFINED:
	case UAT_VENDOR:
	case UATI_UNDEFINED:
/* output terminal types */
	case UATO_UNDEFINED:
	case UATO_DISPLAYAUDIO:
/* bidir terminal types */
	case UATB_UNDEFINED:
	case UATB_HANDSET:
	case UATB_HEADSET:
	case UATB_SPEAKERPHONE:
	case UATB_SPEAKERPHONEESUP:
	case UATB_SPEAKERPHONEECANC:
/* external terminal types */
	case UATE_UNDEFINED:
/* embedded function terminal types */
	case UATF_UNDEFINED:
	case UATF_CALIBNOISE:
	case UATF_EQUNOISE:
	case UATF_DAT:
	case UATF_DCC:
	case UATF_MINIDISK:
	case UATF_ANALOGTAPE:
	case UATF_PHONOGRAPH:
	case UATF_VCRAUDIO:
	case UATF_SATELLITE:
	case UATF_CABLETUNER:
	case UATF_DSS:
	case UATF_RADIORECV:
	case UATF_RADIOXMIT:
	case UATF_MULTITRACK:
	case 0xffff:
	default:
		DPRINTF("'master' for %#.4x\n", terminal_type);
		return AudioNmaster;
	}
	return AudioNmaster;
}

static void
uaudio_add_feature_mixer(struct uaudio_softc *sc, const struct io_terminal *iot,
    int unit, int ctl, struct mixerctl *mc)
{
	const char *mixername, *attr = NULL;
	int terminal_type;

	mc->wIndex = MAKE(unit, sc->sc_ac_iface);
	terminal_type = uaudio_determine_class(iot, mc);
	mixername = uaudio_feature_name(iot, mc->class, terminal_type);
	switch (ctl) {
	case MUTE_CONTROL:
		mc->type = MIX_ON_OFF;
		mc->ctlunit = "";
		attr = AudioNmute;
		break;
	case VOLUME_CONTROL:
		mc->type = MIX_SIGNED_16;
		mc->ctlunit = AudioNvolume;
		attr = NULL;
		break;
	case BASS_CONTROL:
		mc->type = MIX_SIGNED_8;
		mc->ctlunit = AudioNbass;
		attr = AudioNbass;
		break;
	case MID_CONTROL:
		mc->type = MIX_SIGNED_8;
		mc->ctlunit = AudioNmid;
		attr = AudioNmid;
		break;
	case TREBLE_CONTROL:
		mc->type = MIX_SIGNED_8;
		mc->ctlunit = AudioNtreble;
		attr = AudioNtreble;
		break;
	case GRAPHIC_EQUALIZER_CONTROL:
		return; /* XXX don't add anything */
		break;
	case AGC_CONTROL:
		mc->type = MIX_ON_OFF;
		mc->ctlunit = "";
		attr = AudioNagc;
		break;
	case DELAY_CONTROL:
		mc->type = MIX_UNSIGNED_16;
		mc->ctlunit = "4 ms";
		attr = AudioNdelay;
		break;
	case BASS_BOOST_CONTROL:
		mc->type = MIX_ON_OFF;
		mc->ctlunit = "";
		attr = AudioNbassboost;
		break;
	case LOUDNESS_CONTROL:
		mc->type = MIX_ON_OFF;
		mc->ctlunit = "";
		attr = AudioNloudness;
		break;
	case GAIN_CONTROL:
		mc->type = MIX_SIGNED_16;
		mc->ctlunit = "gain";
		attr = "gain";;
		break;
	case GAINPAD_CONTROL:
		mc->type = MIX_SIGNED_16;
		mc->ctlunit = "gainpad";
		attr = "gainpad";;
		break;
	case PHASEINV_CONTROL:
		mc->type = MIX_ON_OFF;
		mc->ctlunit = "";
		attr = "phaseinv";;
		break;
	case UNDERFLOW_CONTROL:
		mc->type = MIX_ON_OFF;
		mc->ctlunit = "";
		attr = "underflow";;
		break;
	case OVERFLOW_CONTROL:
		mc->type = MIX_ON_OFF;
		mc->ctlunit = "";
		attr = "overflow";;
		break;
	default:
		return; /* XXX don't add anything */
		break;
	}

	if (attr != NULL) {
		snprintf(mc->ctlname, sizeof(mc->ctlname),
		    "%s.%s", mixername, attr);
	} else {
		snprintf(mc->ctlname, sizeof(mc->ctlname),
		    "%s", mixername);
	}

	uaudio_mixer_add_ctl(sc, mc);
}

Static void
uaudio_add_feature(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{
	const union usb_audio_feature_unit *d;
	const uByte *ctls;
	const uDWord *ctls2;
	int ctlsize;
	int nchan;
	u_int fumask, mmask, cmask;
	struct mixerctl mix;
	int chan, ctl, i, unit;

	d = iot[id].d.fu;

	switch (sc->sc_version) {
	case UAUDIO_VERSION1:

#define GETV1(i) (ctls[(i)*ctlsize] | \
		(ctlsize > 1 ? ctls[(i)*ctlsize+1] << 8 : 0))

		ctls = d->v1.bmaControls;
		ctlsize = d->v1.bControlSize;
		if (ctlsize == 0) {
			DPRINTF("ignoring feature %d with controlSize of zero\n", id);
			return;
		}

		/* offsetof bmaControls + sizeof iFeature == 7 */
		nchan = (d->v1.bLength - 7) / ctlsize;
		mmask = GETV1(0);
		/* Figure out what we can control */
		for (cmask = 0, chan = 1; chan < nchan; chan++) {
			DPRINTFN(9,"chan=%d mask=%x\n",
				    chan, GETV1(chan));
			cmask |= GETV1(chan);
		}

		DPRINTFN(1,"bUnitId=%d, "
			    "%d channels, mmask=0x%04x, cmask=0x%04x\n",
			    d->v1.bUnitId, nchan, mmask, cmask);

		if (nchan > MIX_MAX_CHAN)
			nchan = MIX_MAX_CHAN;
		unit = d->v1.bUnitId;

		for (ctl = MUTE_CONTROL; ctl <= LOUDNESS_CONTROL; ctl++) {
			fumask = FU_MASK(ctl);
			DPRINTFN(4,"ctl=%d fumask=0x%04x\n",
				    ctl, fumask);
			if (mmask & fumask) {
				mix.nchan = 1;
				mix.wValue[0] = MAKE(ctl, 0);
			} else if (cmask & fumask) {
				mix.nchan = nchan - 1;
				for (i = 1; i < nchan; i++) {
					if (GETV1(i) & fumask)
						mix.wValue[i-1] = MAKE(ctl, i);
					else
						mix.wValue[i-1] = -1;
				}
			} else {
				continue;
			}

			uaudio_add_feature_mixer(sc, &iot[id], unit, ctl, &mix);
		}
#undef GETV1
		break;

	case UAUDIO_VERSION2:

#define GETV2(i) UGETDW(ctls2[(i)])

		ctls2 = d->v2.bmaControls;

		/* offsetof bmaControls + sizeof iFeature == 6 */
		nchan = (d->v2.bLength - 6) / 4;
		if (nchan <= 0) {
			DPRINTF("ignoring feature %d with no controls\n", id);
			return;
		}

		mmask = GETV2(0);
		/* Figure out what we can control */
		for (cmask = 0, chan = 1; chan < nchan; chan++) {
			DPRINTFN(9,"chan=%d mask=%x\n",
				    chan, GETV2(chan));
			cmask |= GETV2(chan);
		}

		DPRINTFN(1,"bUnitId=%d, "
			    "%d channels, mmask=0x%04x, cmask=0x%04x\n",
			    d->v2.bUnitId, nchan, mmask, cmask);

		if (nchan > MIX_MAX_CHAN)
			nchan = MIX_MAX_CHAN;
		unit = d->v2.bUnitId;

		for (ctl = MUTE_CONTROL; ctl <= OVERFLOW_CONTROL; ctl++) {
			fumask = V2_FU_MASK(ctl);
			DPRINTFN(4,"ctl=%d fumask=0x%08x\n",
				    ctl, fumask);

			if (mmask & fumask) {
				mix.nchan = 1;
				mix.wValue[0] = MAKE(ctl, 0);
			} else if (cmask & fumask) {
				mix.nchan = nchan-1;
				for (i = 1; i < nchan; ++i) {
					if (GETV2(i) & fumask)
						mix.wValue[i-1] = MAKE(ctl, i);
					else
						mix.wValue[i-1] = -1;
				}
			} else {
				continue;
			}

			uaudio_add_feature_mixer(sc, &iot[id], unit, ctl, &mix);
		}

#undef GETV2
		break;
	}
}

Static void
uaudio_add_processing_updown(struct uaudio_softc *sc,
			     const struct io_terminal *iot, int id)
{
	const struct usb_audio_processing_unit *d;
	const struct usb_audio_processing_unit_1 *d1;
	const struct usb_audio_processing_unit_updown *ud;
	struct mixerctl mix;
	int i;

	d = iot[id].d.pu;
	d1 = (const struct usb_audio_processing_unit_1 *)
	    &d->baSourceId[d->bNrInPins];
	ud = (const struct usb_audio_processing_unit_updown *)
	    &d1->bmControls[d1->bControlSize];
	DPRINTFN(2,"bUnitId=%d bNrModes=%d\n",
		    d->bUnitId, ud->bNrModes);

	if (!(d1->bmControls[0] & UA_PROC_MASK(UD_MODE_SELECT_CONTROL))) {
		DPRINTF("%s", "no mode select\n");
		return;
	}

	mix.wIndex = MAKE(d->bUnitId, sc->sc_ac_iface);
	mix.nchan = 1;
	mix.wValue[0] = MAKE(UD_MODE_SELECT_CONTROL, 0);
	uaudio_determine_class(&iot[id], &mix);
	mix.type = MIX_ON_OFF;	/* XXX */
	mix.ctlunit = "";
	snprintf(mix.ctlname, sizeof(mix.ctlname), "pro%d-mode", d->bUnitId);

	for (i = 0; i < ud->bNrModes; i++) {
		DPRINTFN(2,"i=%d bm=%#x\n",
			    i, UGETW(ud->waModes[i]));
		/* XXX */
	}
	uaudio_mixer_add_ctl(sc, &mix);
}

Static void
uaudio_add_processing(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{
	const struct usb_audio_processing_unit *d;
	const struct usb_audio_processing_unit_1 *d1;
	int ptype;
	struct mixerctl mix;

	d = iot[id].d.pu;
	d1 = (const struct usb_audio_processing_unit_1 *)
	    &d->baSourceId[d->bNrInPins];
	ptype = UGETW(d->wProcessType);
	DPRINTFN(2,"wProcessType=%d bUnitId=%d "
		    "bNrInPins=%d\n", ptype, d->bUnitId, d->bNrInPins);

	if (d1->bmControls[0] & UA_PROC_ENABLE_MASK) {
		mix.wIndex = MAKE(d->bUnitId, sc->sc_ac_iface);
		mix.nchan = 1;
		mix.wValue[0] = MAKE(XX_ENABLE_CONTROL, 0);
		uaudio_determine_class(&iot[id], &mix);
		mix.type = MIX_ON_OFF;
		mix.ctlunit = "";
		snprintf(mix.ctlname, sizeof(mix.ctlname), "pro%d.%d-enable",
		    d->bUnitId, ptype);
		uaudio_mixer_add_ctl(sc, &mix);
	}

	switch(ptype) {
	case UPDOWNMIX_PROCESS:
		uaudio_add_processing_updown(sc, iot, id);
		break;
	case DOLBY_PROLOGIC_PROCESS:
	case P3D_STEREO_EXTENDER_PROCESS:
	case REVERBATION_PROCESS:
	case CHORUS_PROCESS:
	case DYN_RANGE_COMP_PROCESS:
	default:
#ifdef UAUDIO_DEBUG
		aprint_debug(
		    "uaudio_add_processing: unit %d, type=%d not impl.\n",
		    d->bUnitId, ptype);
#endif
		break;
	}
}

Static void
uaudio_add_effect(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{

#ifdef UAUDIO_DEBUG
	aprint_debug("uaudio_add_effect: not impl.\n");
#endif
}

Static void
uaudio_add_extension(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{
	const struct usb_audio_extension_unit *d;
	const struct usb_audio_extension_unit_1 *d1;
	struct mixerctl mix;

	d = iot[id].d.eu;
	d1 = (const struct usb_audio_extension_unit_1 *)
	    &d->baSourceId[d->bNrInPins];
	DPRINTFN(2,"bUnitId=%d bNrInPins=%d\n",
		    d->bUnitId, d->bNrInPins);

	if (usbd_get_quirks(sc->sc_udev)->uq_flags & UQ_AU_NO_XU)
		return;

	if (d1->bmControls[0] & UA_EXT_ENABLE_MASK) {
		mix.wIndex = MAKE(d->bUnitId, sc->sc_ac_iface);
		mix.nchan = 1;
		mix.wValue[0] = MAKE(UA_EXT_ENABLE, 0);
		uaudio_determine_class(&iot[id], &mix);
		mix.type = MIX_ON_OFF;
		mix.ctlunit = "";
		snprintf(mix.ctlname, sizeof(mix.ctlname), "ext%d-enable",
		    d->bUnitId);
		uaudio_mixer_add_ctl(sc, &mix);
	}
}

Static void
uaudio_add_clksrc(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{
	const struct usb_audio_clksrc_unit *d;
	struct mixerctl mix;

	d = iot[id].d.cu;
	DPRINTFN(2,"bClockId=%d bmAttributes=%d bmControls=%d bAssocTerminal=%d iClockSource=%d\n",
		    d->bClockId, d->bmAttributes, d->bmControls, d->bAssocTerminal, d->iClockSource);
	mix.wIndex = MAKE(d->bClockId, sc->sc_ac_iface);
	uaudio_determine_class(&iot[id], &mix);
	mix.nchan = 1;
	mix.wValue[0] = MAKE(V2_CUR_CLKFREQ, 0);
	mix.type = MIX_UNSIGNED_32;
	mix.ctlunit = "";

	uaudio_makename(sc, d->iClockSource, uaudio_clockname(d->bmAttributes),
	    d->bClockId, mix.ctlname, sizeof(mix.ctlname));
	uaudio_mixer_add_ctl(sc, &mix);
}

Static void
uaudio_add_clksel(struct uaudio_softc *sc, const struct io_terminal *iot, int id)
{
	const struct usb_audio_clksel_unit *d;
	struct mixerctl mix;
	int i, wp;
	uByte sel;

	d = iot[id].d.lu;
	sel = ((const uByte *)&d->baCSourceId[d->bNrInPins])[2]; /* iClockSelector */
	DPRINTFN(2,"bClockId=%d bNrInPins=%d iClockSelector=%d\n",
		    d->bClockId, d->bNrInPins, sel);
	mix.wIndex = MAKE(d->bClockId, sc->sc_ac_iface);
	uaudio_determine_class(&iot[id], &mix);
	mix.nchan = 1;
	mix.wValue[0] = MAKE(V2_CUR_CLKSEL, 0);
	mix.type = MIX_SELECTOR;
	mix.ctlunit = "";
	mix.range0.minval = 1;
	mix.range0.maxval = d->bNrInPins;
	mix.range0.resval = 1;
	mix.mul = mix.range0.maxval - mix.range0.minval;
	wp = uaudio_makename(sc, sel, "clksel", d->bClockId, mix.ctlname, MAX_AUDIO_DEV_LEN);
	for (i = 1; i <= d->bNrInPins; i++) {
		wp += snprintf(mix.ctlname + wp, MAX_AUDIO_DEV_LEN - wp,
			       "%si%d", i == 1 ? "-" : "", d->baCSourceId[i - 1]);
		if (wp > MAX_AUDIO_DEV_LEN - 1)
			break;
	}
	uaudio_mixer_add_ctl(sc, &mix);
}

Static struct terminal_list*
uaudio_merge_terminal_list(const struct io_terminal *iot)
{
	struct terminal_list *tml;
	uint16_t *ptm;
	int i, len;

	len = 0;
	if (iot->inputs == NULL)
		return NULL;
	for (i = 0; i < iot->inputs_size; i++) {
		if (iot->inputs[i] != NULL)
			len += iot->inputs[i]->size;
	}
	tml = malloc(TERMINAL_LIST_SIZE(len), M_TEMP, M_NOWAIT);
	if (tml == NULL) {
		aprint_error("uaudio_merge_terminal_list: no memory\n");
		return NULL;
	}
	tml->size = 0;
	ptm = tml->terminals;
	for (i = 0; i < iot->inputs_size; i++) {
		if (iot->inputs[i] == NULL)
			continue;
		if (iot->inputs[i]->size > len)
			break;
		memcpy(ptm, iot->inputs[i]->terminals,
		       iot->inputs[i]->size * sizeof(uint16_t));
		tml->size += iot->inputs[i]->size;
		ptm += iot->inputs[i]->size;
		len -= iot->inputs[i]->size;
	}
	return tml;
}

Static struct terminal_list *
uaudio_io_terminaltype(struct uaudio_softc *sc, int outtype, struct io_terminal *iot, int id)
{
	struct terminal_list *tml;
	struct io_terminal *it;
	int src_id, i;

	it = &iot[id];
	if (it->output != NULL) {
		/* already has outtype? */
		for (i = 0; i < it->output->size; i++)
			if (it->output->terminals[i] == outtype)
				return uaudio_merge_terminal_list(it);
		tml = malloc(TERMINAL_LIST_SIZE(it->output->size + 1),
			     M_TEMP, M_NOWAIT);
		if (tml == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return uaudio_merge_terminal_list(it);
		}
		memcpy(tml, it->output, TERMINAL_LIST_SIZE(it->output->size));
		tml->terminals[it->output->size] = outtype;
		tml->size++;
		free(it->output, M_TEMP);
		it->output = tml;
		if (it->inputs != NULL) {
			for (i = 0; i < it->inputs_size; i++)
				if (it->inputs[i] != NULL)
					free(it->inputs[i], M_TEMP);
			free(it->inputs, M_TEMP);
		}
		it->inputs_size = 0;
		it->inputs = NULL;
	} else {		/* end `iot[id] != NULL' */
		it->inputs_size = 0;
		it->inputs = NULL;
		it->output = malloc(TERMINAL_LIST_SIZE(1), M_TEMP, M_NOWAIT);
		if (it->output == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return NULL;
		}
		it->output->terminals[0] = outtype;
		it->output->size = 1;
		it->direct = FALSE;
	}

	switch (it->d.desc->bDescriptorSubtype) {
	case UDESCSUB_AC_INPUT:
		it->inputs = malloc(sizeof(struct terminal_list *), M_TEMP, M_NOWAIT);
		if (it->inputs == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return NULL;
		}
		tml = malloc(TERMINAL_LIST_SIZE(1), M_TEMP, M_NOWAIT);
		if (tml == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			free(it->inputs, M_TEMP);
			it->inputs = NULL;
			return NULL;
		}
		it->inputs[0] = tml;
		switch (sc->sc_version) {
		case UAUDIO_VERSION1:
			tml->terminals[0] = UGETW(it->d.it->v1.wTerminalType);
			break;
		case UAUDIO_VERSION2:
			tml->terminals[0] = UGETW(it->d.it->v2.wTerminalType);
			break;
		default:
			free(tml, M_TEMP);
			free(it->inputs, M_TEMP);
			it->inputs = NULL;
			return NULL;
		}
		tml->size = 1;
		it->inputs_size = 1;
		return uaudio_merge_terminal_list(it);
	case UDESCSUB_AC_FEATURE:
		switch (sc->sc_version) {
		case UAUDIO_VERSION1:
			src_id = it->d.fu->v1.bSourceId;
			break;
		case UAUDIO_VERSION2:
			src_id = it->d.fu->v2.bSourceId;
			break;
		default:
			/* cannot happen */
			return NULL;
		}
		it->inputs = malloc(sizeof(struct terminal_list *), M_TEMP, M_NOWAIT);
		if (it->inputs == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return uaudio_io_terminaltype(sc, outtype, iot, src_id);
		}
		it->inputs[0] = uaudio_io_terminaltype(sc, outtype, iot, src_id);
		it->inputs_size = 1;
		return uaudio_merge_terminal_list(it);
	case UDESCSUB_AC_OUTPUT:
		it->inputs = malloc(sizeof(struct terminal_list *), M_TEMP, M_NOWAIT);
		if (it->inputs == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return NULL;
		}
		switch (sc->sc_version) {
		case UAUDIO_VERSION1:
			src_id = it->d.ot->v1.bSourceId;
			break;
		case UAUDIO_VERSION2:
			src_id = it->d.ot->v2.bSourceId;
			break;
		default:
			free(it->inputs, M_TEMP);
			it->inputs = NULL;
			return NULL;
		}
		it->inputs[0] = uaudio_io_terminaltype(sc, outtype, iot, src_id);
		it->inputs_size = 1;
		iot[src_id].direct = TRUE;
		return NULL;
	case UDESCSUB_AC_MIXER:
		it->inputs_size = 0;
		it->inputs = malloc(sizeof(struct terminal_list *)
				    * it->d.mu->bNrInPins, M_TEMP, M_NOWAIT);
		if (it->inputs == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return NULL;
		}
		for (i = 0; i < it->d.mu->bNrInPins; i++) {
			src_id = it->d.mu->baSourceId[i];
			it->inputs[i] = uaudio_io_terminaltype(sc, outtype, iot,
							       src_id);
			it->inputs_size++;
		}
		return uaudio_merge_terminal_list(it);
	case UDESCSUB_AC_SELECTOR:
		it->inputs_size = 0;
		it->inputs = malloc(sizeof(struct terminal_list *)
				    * it->d.su->bNrInPins, M_TEMP, M_NOWAIT);
		if (it->inputs == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return NULL;
		}
		for (i = 0; i < it->d.su->bNrInPins; i++) {
			src_id = it->d.su->baSourceId[i];
			it->inputs[i] = uaudio_io_terminaltype(sc, outtype, iot,
							       src_id);
			it->inputs_size++;
		}
		return uaudio_merge_terminal_list(it);
	case UDESCSUB_AC_PROCESSING:
		it->inputs_size = 0;
		it->inputs = malloc(sizeof(struct terminal_list *)
				    * it->d.pu->bNrInPins, M_TEMP, M_NOWAIT);
		if (it->inputs == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return NULL;
		}
		for (i = 0; i < it->d.pu->bNrInPins; i++) {
			src_id = it->d.pu->baSourceId[i];
			it->inputs[i] = uaudio_io_terminaltype(sc, outtype, iot,
							       src_id);
			it->inputs_size++;
		}
		return uaudio_merge_terminal_list(it);
	case UDESCSUB_AC_EXTENSION:
		it->inputs_size = 0;
		it->inputs = malloc(sizeof(struct terminal_list *)
				    * it->d.eu->bNrInPins, M_TEMP, M_NOWAIT);
		if (it->inputs == NULL) {
			aprint_error("uaudio_io_terminaltype: no memory\n");
			return NULL;
		}
		for (i = 0; i < it->d.eu->bNrInPins; i++) {
			src_id = it->d.eu->baSourceId[i];
			it->inputs[i] = uaudio_io_terminaltype(sc, outtype, iot,
							       src_id);
			it->inputs_size++;
		}
		return uaudio_merge_terminal_list(it);
	case UDESCSUB_AC_HEADER:
	default:
		return NULL;
	}
}

Static usbd_status
uaudio_identify(struct uaudio_softc *sc, const usb_config_descriptor_t *cdesc)
{
	usbd_status err;

	err = uaudio_identify_ac(sc, cdesc);
	if (err)
		return err;
	err = uaudio_identify_as(sc, cdesc);
	if (err)
		return err;

	uaudio_build_formats(sc);
	return 0;
}

Static void
uaudio_add_alt(struct uaudio_softc *sc, const struct as_info *ai)
{
	size_t len;
	struct as_info *nai;

	len = sizeof(*ai) * (sc->sc_nalts + 1);
	nai = kmem_alloc(len, KM_SLEEP);
	/* Copy old data, if there was any */
	if (sc->sc_nalts != 0) {
		memcpy(nai, sc->sc_alts, sizeof(*ai) * (sc->sc_nalts));
		kmem_free(sc->sc_alts, sizeof(*ai) * sc->sc_nalts);
	}
	sc->sc_alts = nai;
	DPRINTFN(2,"adding alt=%d, enc=%d\n",
		    ai->alt, ai->encoding);
	sc->sc_alts[sc->sc_nalts++] = *ai;
}

Static usbd_status
uaudio_process_as(struct uaudio_softc *sc, const char *tbuf, int *offsp,
		  int size, const usb_interface_descriptor_t *id)
{
	const union usb_audio_streaming_interface_descriptor *asid;
	const union usb_audio_streaming_type1_descriptor *asf1d;
	const usb_endpoint_descriptor_audio_t *ed;
	const usb_endpoint_descriptor_audio_t *epdesc1;
	const struct usb_audio_streaming_endpoint_descriptor *sed;
	int format, chan __unused, prec, bps, enc, terminal;
	int dir, type, sync, epcount;
	struct as_info ai;
	const char *format_str __unused;
	const uaudio_cs_descriptor_t *desc;

	DPRINTF("offset = %d < %d\n", *offsp, size);

	epcount = 0;
	asid = NULL;
	asf1d = NULL;
	ed = NULL;
	epdesc1 = NULL;
	sed = NULL;

	while (*offsp < size) {
		desc = (const uaudio_cs_descriptor_t *)(tbuf + *offsp);
		if (*offsp + desc->bLength > size)
			return USBD_INVAL;

		switch (desc->bDescriptorType) {
		case UDESC_CS_INTERFACE:
			switch (desc->bDescriptorSubtype) {
			case AS_GENERAL:
				if (asid != NULL)
					goto ignore;
				asid = (const union usb_audio_streaming_interface_descriptor *) desc;
				DPRINTF("asid: bTerminalLink=%d wFormatTag=%d bmFormats=0x%x bLength=%d\n",
					 asid->v1.bTerminalLink, UGETW(asid->v1.wFormatTag),
					UGETDW(asid->v2.bmFormats), asid->v1.bLength);
				break;
			case FORMAT_TYPE:
				if (asf1d != NULL)
					goto ignore;
				asf1d = (const union usb_audio_streaming_type1_descriptor *) desc;
				DPRINTF("asf1d: bDescriptorType=%d bDescriptorSubtype=%d\n",
				         asf1d->v1.bDescriptorType, asf1d->v1.bDescriptorSubtype);
				if (asf1d->v1.bFormatType != FORMAT_TYPE_I) {
					aprint_normal_dev(sc->sc_dev,
					    "ignored setting with type %d format\n", asf1d->v1.bFormatType);
					return USBD_NORMAL_COMPLETION;
				}
				break;
			default:
				goto ignore;
			}
			break;
		case UDESC_ENDPOINT:
			epcount++;
			if (epcount > id->bNumEndpoints)
				goto ignore;
			switch (epcount) {
			case 1:
				ed = (const usb_endpoint_descriptor_audio_t *) desc;
				DPRINTF("endpoint[0] bLength=%d bDescriptorType=%d "
					 "bEndpointAddress=%d bmAttributes=%#x wMaxPacketSize=%d "
					 "bInterval=%d bRefresh=%d bSynchAddress=%d\n",
					 ed->bLength, ed->bDescriptorType, ed->bEndpointAddress,
					 ed->bmAttributes, UGETW(ed->wMaxPacketSize),
					 ed->bInterval,
					 ed->bLength > 7 ? ed->bRefresh : 0,
					 ed->bLength > 8 ? ed->bSynchAddress : 0);
				if (UE_GET_XFERTYPE(ed->bmAttributes) != UE_ISOCHRONOUS)
					return USBD_INVAL;
				break;
			case 2:
				epdesc1 = (const usb_endpoint_descriptor_audio_t *) desc;
				DPRINTF("endpoint[1] bLength=%d "
					 "bDescriptorType=%d bEndpointAddress=%d "
					 "bmAttributes=%#x wMaxPacketSize=%d bInterval=%d "
					 "bRefresh=%d bSynchAddress=%d\n",
					 epdesc1->bLength, epdesc1->bDescriptorType,
					 epdesc1->bEndpointAddress, epdesc1->bmAttributes,
					 UGETW(epdesc1->wMaxPacketSize), epdesc1->bInterval,
					 epdesc1->bLength > 7 ? epdesc1->bRefresh : 0,
					 epdesc1->bLength > 8 ? epdesc1->bSynchAddress : 0);
#if 0
				if (epdesc1->bLength > 8 && epdesc1->bSynchAddress != 0) {
					aprint_error_dev(sc->sc_dev,
					    "invalid endpoint: bSynchAddress=0\n");
					return USBD_INVAL;
				}
#endif
				if (UE_GET_XFERTYPE(epdesc1->bmAttributes) != UE_ISOCHRONOUS) {
					aprint_error_dev(sc->sc_dev,
					    "invalid endpoint: bmAttributes=%#x\n",
					     epdesc1->bmAttributes);
					return USBD_INVAL;
				}
#if 0
				if (ed->bLength > 8 && epdesc1->bEndpointAddress != ed->bSynchAddress) {
					aprint_error_dev(sc->sc_dev,
					    "invalid endpoint addresses: "
					    "ep[0]->bSynchAddress=%#x "
					    "ep[1]->bEndpointAddress=%#x\n",
					    ed->bSynchAddress, epdesc1->bEndpointAddress);
					return USBD_INVAL;
				}
#endif
				/* UE_GET_ADDR(epdesc1->bEndpointAddress), and epdesc1->bRefresh */
				break;
			default:
				goto ignore;
			}
			break;
		case UDESC_CS_ENDPOINT:
			switch (desc->bDescriptorSubtype) {
			case AS_GENERAL:
				if (sed != NULL)
					goto ignore;
				sed = (const struct usb_audio_streaming_endpoint_descriptor *) desc;
				DPRINTF(" streadming_endpoint: offset=%d bLength=%d\n", *offsp, sed->bLength);
				break;
			default:
				goto ignore;
			}
			break;
		case UDESC_INTERFACE:
		case UDESC_DEVICE:
			goto leave;
		default:
ignore:
			aprint_normal_dev(sc->sc_dev,
			    "ignored descriptor type %d subtype %d\n",
			    desc->bDescriptorType, desc->bDescriptorSubtype);
			break;
		}

		*offsp += desc->bLength;
	}
leave:

	if (asid == NULL) {
		DPRINTF("%s", "No streaming interface descriptor found\n");
		return USBD_INVAL;
	}
	if (asf1d == NULL) {
		DPRINTF("%s", "No format type descriptor found\n");
		return USBD_INVAL;
	}
	if (ed == NULL) {
		DPRINTF("%s", "No endpoint descriptor found\n");
		return USBD_INVAL;
	}
	if (sed == NULL) {
		DPRINTF("%s", "No streaming endpoint descriptor found\n");
		return USBD_INVAL;
	}

	dir = UE_GET_DIR(ed->bEndpointAddress);
	type = UE_GET_ISO_TYPE(ed->bmAttributes);
	if ((usbd_get_quirks(sc->sc_udev)->uq_flags & UQ_AU_INP_ASYNC) &&
	    dir == UE_DIR_IN && type == UE_ISO_ADAPT)
		type = UE_ISO_ASYNC;
	/* We can't handle endpoints that need a sync pipe yet. */
	sync = FALSE;
	if (dir == UE_DIR_IN && type == UE_ISO_ADAPT) {
		sync = TRUE;
#ifndef UAUDIO_MULTIPLE_ENDPOINTS
		aprint_normal_dev(sc->sc_dev,
		    "ignored input endpoint of type adaptive\n");
		return USBD_NORMAL_COMPLETION;
#endif
	}
	if (dir != UE_DIR_IN && type == UE_ISO_ASYNC) {
		sync = TRUE;
#ifndef UAUDIO_MULTIPLE_ENDPOINTS
		aprint_normal_dev(sc->sc_dev,
		    "ignored output endpoint of type async\n");
		return USBD_NORMAL_COMPLETION;
#endif
	}
#ifdef UAUDIO_MULTIPLE_ENDPOINTS
	if (sync && id->bNumEndpoints <= 1) {
		aprint_error_dev(sc->sc_dev,
		    "a sync-pipe endpoint but no other endpoint\n");
		return USBD_INVAL;
	}
#endif
	if (!sync && id->bNumEndpoints > 1) {
		aprint_error_dev(sc->sc_dev,
		    "non sync-pipe endpoint but multiple endpoints\n");
		return USBD_INVAL;
	}

	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
		format = UGETW(asid->v1.wFormatTag);
		chan = asf1d->v1.bNrChannels;
		prec = asf1d->v1.bBitResolution;
		bps = asf1d->v1.bSubFrameSize;
		break;
	case UAUDIO_VERSION2:
		format = UGETDW(asid->v2.bmFormats);
		chan = asid->v2.bNrChannels;
		prec = asf1d->v2.bBitResolution;
		bps = asf1d->v2.bSubslotSize;
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "Unknown audio class %d\n", sc->sc_version);
		return USBD_INVAL;
	}
	if ((prec != 8 && prec != 16 && prec != 24 && prec != 32) || (bps < 1 || bps > 4)) {
		aprint_normal_dev(sc->sc_dev,
		    "ignored setting with precision %d bps %d\n", prec, bps);
		return USBD_NORMAL_COMPLETION;
	}
	enc = AUDIO_ENCODING_NONE;
	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
		terminal = 0;
		switch (format) {
		case UA_FMT_PCM:
			if (prec == 8) {
				sc->sc_altflags |= HAS_8;
			} else if (prec == 16) {
				sc->sc_altflags |= HAS_16;
			} else if (prec == 24) {
				sc->sc_altflags |= HAS_24;
			} else if (prec == 32) {
				sc->sc_altflags |= HAS_32;
			}
			enc = AUDIO_ENCODING_SLINEAR_LE;
			format_str = "pcm";
			break;
		case UA_FMT_PCM8:
			enc = AUDIO_ENCODING_ULINEAR_LE;
			sc->sc_altflags |= HAS_8U;
			format_str = "pcm8";
			break;
		case UA_FMT_ALAW:
			enc = AUDIO_ENCODING_ALAW;
			sc->sc_altflags |= HAS_ALAW;
			format_str = "alaw";
			break;
		case UA_FMT_MULAW:
			enc = AUDIO_ENCODING_ULAW;
			sc->sc_altflags |= HAS_MULAW;
			format_str = "mulaw";
			break;
#ifdef notyet
		case UA_FMT_IEEE_FLOAT:
			break;
#endif
		}
		break;
	case UAUDIO_VERSION2:
		terminal = asid->v2.bTerminalLink;
		if (format & UA_V2_FMT_PCM) {
			if (prec == 8) {
				sc->sc_altflags |= HAS_8;
			} else if (prec == 16) {
				sc->sc_altflags |= HAS_16;
			} else if (prec == 24) {
				sc->sc_altflags |= HAS_24;
			} else if (prec == 32) {
				sc->sc_altflags |= HAS_32;
			}
			enc = AUDIO_ENCODING_SLINEAR_LE;
			format_str = "pcm";
		} else if (format & UA_V2_FMT_PCM8) {
			enc = AUDIO_ENCODING_ULINEAR_LE;
			sc->sc_altflags |= HAS_8U;
			format_str = "pcm8";
		} else if (format & UA_V2_FMT_ALAW) {
			enc = AUDIO_ENCODING_ALAW;
			sc->sc_altflags |= HAS_ALAW;
			format_str = "alaw";
		} else if (format & UA_V2_FMT_MULAW) {
			enc = AUDIO_ENCODING_ULAW;
			sc->sc_altflags |= HAS_MULAW;
			format_str = "mulaw";
#ifdef notyet
		} else if (format & UA_V2_FMT_IEEE_FLOAT) {
#endif
		}
		break;
	}
	if (enc == AUDIO_ENCODING_NONE) {
		aprint_normal_dev(sc->sc_dev,
		    "ignored setting with format 0x%08x\n", format);
		return USBD_NORMAL_COMPLETION;
	}
#ifdef UAUDIO_DEBUG
	aprint_debug_dev(sc->sc_dev, "%s: %dch, %d/%dbit, %s,",
	       dir == UE_DIR_IN ? "recording" : "playback",
	       chan, prec, bps * 8, format_str);
	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
		if (asf1d->v1.bSamFreqType == UA_SAMP_CONTINUOUS) {
			aprint_debug(" %d-%dHz\n", UA_SAMP_LO(&asf1d->v1),
			    UA_SAMP_HI(&asf1d->v1));
		} else {
			int r;
			aprint_debug(" %d", UA_GETSAMP(&asf1d->v1, 0));
			for (r = 1; r < asf1d->v1.bSamFreqType; r++)
				aprint_debug(",%d", UA_GETSAMP(&asf1d->v1, r));
			aprint_debug("Hz\n");
		}
		break;
	/* UAUDIO_VERSION2 has no frequency information in the format */
	}
#endif
	ai.alt = id->bAlternateSetting;
	ai.encoding = enc;
	ai.attributes = sed->bmAttributes;
	ai.idesc = id;
	ai.edesc = ed;
	ai.edesc1 = epdesc1;
	ai.asf1desc = asf1d;
	ai.sc_busy = 0;
	ai.nchan = chan;
	ai.aformat = NULL;
	ai.ifaceh = NULL;
	ai.terminal = terminal;
	uaudio_add_alt(sc, &ai);
#ifdef UAUDIO_DEBUG
	if (ai.attributes & UA_SED_FREQ_CONTROL)
		DPRINTFN(1, "%s", "FREQ_CONTROL\n");
	if (ai.attributes & UA_SED_PITCH_CONTROL)
		DPRINTFN(1, "%s", "PITCH_CONTROL\n");
#endif
	sc->sc_mode |= (dir == UE_DIR_OUT) ? AUMODE_PLAY : AUMODE_RECORD;

	return USBD_NORMAL_COMPLETION;
}

Static usbd_status
uaudio_identify_as(struct uaudio_softc *sc,
		   const usb_config_descriptor_t *cdesc)
{
	const usb_interface_descriptor_t *id;
	const char *tbuf;
	int size, offs;

	size = UGETW(cdesc->wTotalLength);
	tbuf = (const char *)cdesc;

	/* Locate the AudioStreaming interface descriptor. */
	offs = 0;
	id = uaudio_find_iface(tbuf, size, &offs, UISUBCLASS_AUDIOSTREAM);
	if (id == NULL)
		return USBD_INVAL;

	/* Loop through all the alternate settings. */
	while (offs <= size) {
		DPRINTFN(2, "interface=%d offset=%d\n",
		    id->bInterfaceNumber, offs);
		switch (id->bNumEndpoints) {
		case 0:
			DPRINTFN(2, "AS null alt=%d\n",
				     id->bAlternateSetting);
			sc->sc_nullalt = id->bAlternateSetting;
			break;
		case 1:
#ifdef UAUDIO_MULTIPLE_ENDPOINTS
		case 2:
#endif
			uaudio_process_as(sc, tbuf, &offs, size, id);
			break;
		default:
			aprint_error_dev(sc->sc_dev,
			    "ignored audio interface with %d endpoints\n",
			     id->bNumEndpoints);
			break;
		}
		id = uaudio_find_iface(tbuf, size, &offs, UISUBCLASS_AUDIOSTREAM);
		if (id == NULL)
			break;
	}
	if (offs > size)
		return USBD_INVAL;
	DPRINTF("%d alts available\n", sc->sc_nalts);

	if (sc->sc_mode == 0) {
		aprint_error_dev(sc->sc_dev, "no usable endpoint found\n");
		return USBD_INVAL;
	}

	if (sc->sc_nalts == 0) {
		aprint_error_dev(sc->sc_dev, "no audio formats found\n");
		return USBD_INVAL;
	}

	return USBD_NORMAL_COMPLETION;
}


Static u_int
uaudio_get_rates(struct uaudio_softc *sc, int mode, u_int *freqs, u_int len)
{
	struct mixerctl *mc;
	u_int freq, start, end, step;
	u_int i, n;
	u_int k, count;
	int j;

	/*
	 * With UAC2 the sample rate isn't part of the data format,
	 * instead, you have separate clock sources that may be
	 * assigned to individual terminals (inputs, outputs).
	 *
	 * For audio(4) we only distinguish between input and output
	 * formats and collect the unique rates from all possible clock
	 * sources.
	 */
	n = 0;
	for (j = 0; j < sc->sc_nratectls; ++j) {

		/*
		 * skip rates not associated with a terminal
		 * of the required mode (record/play)
		 */
		if ((sc->sc_ratemode[j] & mode) == 0)
			continue;

		mc = &sc->sc_ctls[sc->sc_ratectls[j]];
		count = mc->nranges ? mc->nranges : 1;
		for (k = 0; k < count; ++k) {
			start = (u_int) mc->ranges[k].minval;
			end   = (u_int) mc->ranges[k].maxval;
			step  = (u_int) mc->ranges[k].resval;
			for (freq = start; freq <= end; freq += step) {
				/* remove duplicates */
				for (i = 0; i < n; ++i) {
					if (freqs[i] == freq)
						break;
				}
				if (i < n) {
					if (step == 0)
						break;
					continue;
				}

				/* store or count */
				if (len != 0) {
					if (n >= len)
						goto done;
					freqs[n] = freq;
				}
				++n;
				if (step == 0)
					break;
			}
		}
	}

done:
	return n;
}

Static void
uaudio_build_formats(struct uaudio_softc *sc)
{
	struct audio_format *auf;
	const struct as_info *as;
	const union usb_audio_streaming_type1_descriptor *t1desc;
	int i, j;

	/* build audio_format array */
	sc->sc_formats = kmem_zalloc(sizeof(struct audio_format) * sc->sc_nalts,
	    KM_SLEEP);
	sc->sc_nformats = sc->sc_nalts;

	for (i = 0; i < sc->sc_nalts; i++) {
		auf = &sc->sc_formats[i];
		as = &sc->sc_alts[i];
		t1desc = as->asf1desc;
		if (UE_GET_DIR(as->edesc->bEndpointAddress) == UE_DIR_OUT)
			auf->mode = AUMODE_PLAY;
		else
			auf->mode = AUMODE_RECORD;
		auf->encoding = as->encoding;
		auf->channel_mask = sc->sc_channel_config;

		switch (sc->sc_version) {
		case UAUDIO_VERSION1:
			auf->validbits = t1desc->v1.bBitResolution;
			auf->precision = t1desc->v1.bSubFrameSize * 8;
			auf->channels = t1desc->v1.bNrChannels;

			auf->frequency_type = t1desc->v1.bSamFreqType;
			if (t1desc->v1.bSamFreqType == UA_SAMP_CONTINUOUS) {
				auf->frequency[0] = UA_SAMP_LO(&t1desc->v1);
				auf->frequency[1] = UA_SAMP_HI(&t1desc->v1);
			} else {
				for (j = 0; j  < t1desc->v1.bSamFreqType; j++) {
					if (j >= AUFMT_MAX_FREQUENCIES) {
						aprint_error("%s: please increase "
						       "AUFMT_MAX_FREQUENCIES to %d\n",
						       __func__, t1desc->v1.bSamFreqType);
						auf->frequency_type =
						    AUFMT_MAX_FREQUENCIES;
						break;
					}
					auf->frequency[j] = UA_GETSAMP(&t1desc->v1, j);
				}
			}
			break;
		case UAUDIO_VERSION2:
			auf->validbits = t1desc->v2.bBitResolution;
			auf->precision = t1desc->v2.bSubslotSize * 8;
			auf->channels = as->nchan;

#if 0
			auf->frequency_type = uaudio_get_rates(sc, auf->mode, NULL, 0);
			if (auf->frequency_type >= AUFMT_MAX_FREQUENCIES) {
				aprint_error("%s: please increase "
				       "AUFMT_MAX_FREQUENCIES to %d\n",
				       __func__, auf->frequency_type);
			}
#endif

			auf->frequency_type = uaudio_get_rates(sc,
			    auf->mode, auf->frequency, AUFMT_MAX_FREQUENCIES);

			/*
			 * if rate query failed, guess a rate
			 */
			if (auf->frequency_type == UA_SAMP_CONTINUOUS) {
				auf->frequency[0] = 48000;
				auf->frequency[1] = 48000;
			}

			break;
		}

		DPRINTF("alt[%d] = %d/%d %dch %u[%u,%u,...] alt %u\n", i,
		    auf->validbits, auf->precision, auf->channels, auf->frequency_type,
		    auf->frequency[0], auf->frequency[1],
		    as->idesc->bAlternateSetting);

		sc->sc_alts[i].aformat = auf;
	}
}

#ifdef UAUDIO_DEBUG
Static void
uaudio_dump_tml(struct terminal_list *tml) {
	if (tml == NULL) {
		printf("NULL");
	} else {
                int i;
		for (i = 0; i < tml->size; i++)
			printf("%s ", uaudio_get_terminal_name
			       (tml->terminals[i]));
	}
	printf("\n");
}
#endif

Static usbd_status
uaudio_identify_ac(struct uaudio_softc *sc, const usb_config_descriptor_t *cdesc)
{
	struct io_terminal* iot;
	const usb_interface_descriptor_t *id;
	const struct usb_audio_control_descriptor *acdp;
	const uaudio_cs_descriptor_t *dp;
	const union usb_audio_output_terminal *pot;
	struct terminal_list *tml;
	const char *tbuf, *ibuf, *ibufend;
	int size, offs, ndps, i, j;

	size = UGETW(cdesc->wTotalLength);
	tbuf = (const char *)cdesc;

	/* Locate the AudioControl interface descriptor. */
	offs = 0;
	id = uaudio_find_iface(tbuf, size, &offs, UISUBCLASS_AUDIOCONTROL);
	if (id == NULL)
		return USBD_INVAL;
	if (offs + sizeof(*acdp) > size)
		return USBD_INVAL;
	sc->sc_ac_iface = id->bInterfaceNumber;
	DPRINTFN(2,"AC interface is %d\n", sc->sc_ac_iface);

	/* A class-specific AC interface header should follow. */
	ibuf = tbuf + offs;
	ibufend = tbuf + size;
	acdp = (const struct usb_audio_control_descriptor *)ibuf;
	if (acdp->bDescriptorType != UDESC_CS_INTERFACE ||
	    acdp->bDescriptorSubtype != UDESCSUB_AC_HEADER)
		return USBD_INVAL;

	if (!(usbd_get_quirks(sc->sc_udev)->uq_flags & UQ_BAD_ADC)) {
		sc->sc_version = UGETW(acdp->bcdADC);
	} else {
		sc->sc_version = UAUDIO_VERSION1;
	}

	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
	case UAUDIO_VERSION2:
		break;
	default:
		return USBD_INVAL;
	}

	sc->sc_audio_rev = UGETW(acdp->bcdADC);
	DPRINTFN(2, "found AC header, vers=%03x\n", sc->sc_audio_rev);

	sc->sc_nullalt = -1;

	/* Scan through all the AC specific descriptors */
	dp = (const uaudio_cs_descriptor_t *)ibuf;
	ndps = 0;
	iot = malloc(sizeof(struct io_terminal) * 256, M_TEMP, M_NOWAIT | M_ZERO);
	if (iot == NULL) {
		aprint_error("%s: no memory\n", __func__);
		return USBD_NOMEM;
	}
	for (;;) {
		ibuf += dp->bLength;
		if (ibuf >= ibufend)
			break;
		dp = (const uaudio_cs_descriptor_t *)ibuf;
		if (ibuf + dp->bLength > ibufend) {
			free(iot, M_TEMP);
			return USBD_INVAL;
		}
		if (dp->bDescriptorType != UDESC_CS_INTERFACE)
			break;
		switch (sc->sc_version) {
		case UAUDIO_VERSION1:
			i = ((const union usb_audio_input_terminal *)dp)->v1.bTerminalId;
			break;
		case UAUDIO_VERSION2:
			i = ((const union usb_audio_input_terminal *)dp)->v2.bTerminalId;
			break;
		default:
			free(iot, M_TEMP);
			return USBD_INVAL;
		}
		iot[i].d.desc = dp;
		if (i > ndps)
			ndps = i;
	}
	ndps++;

	/* construct io_terminal */
	for (i = 0; i < ndps; i++) {
		dp = iot[i].d.desc;
		if (dp == NULL)
			continue;
		if (dp->bDescriptorSubtype != UDESCSUB_AC_OUTPUT)
			continue;
		pot = iot[i].d.ot;
		switch (sc->sc_version) {
		case UAUDIO_VERSION1:
			tml = uaudio_io_terminaltype(sc, UGETW(pot->v1.wTerminalType), iot, i);
			break;
		case UAUDIO_VERSION2:
			tml = uaudio_io_terminaltype(sc, UGETW(pot->v2.wTerminalType), iot, i);
			break;
		default:
			tml = NULL;
			break;
		}
		if (tml != NULL)
			free(tml, M_TEMP);
	}

#ifdef UAUDIO_DEBUG
	for (i = 0; i < 256; i++) {
		union usb_audio_cluster cluster;

		if (iot[i].d.desc == NULL)
			continue;
		printf("id %d:\t", i);
		switch (iot[i].d.desc->bDescriptorSubtype) {
		case UDESCSUB_AC_INPUT:
			printf("AC_INPUT type=%s\n", uaudio_get_terminal_name
				  (UGETW(iot[i].d.it->v1.wTerminalType)));
			printf("\t");
			cluster = uaudio_get_cluster(sc, i, iot);
			uaudio_dump_cluster(sc, &cluster);
			printf("\n");
			break;
		case UDESCSUB_AC_OUTPUT:
			printf("AC_OUTPUT type=%s ", uaudio_get_terminal_name
				  (UGETW(iot[i].d.ot->v1.wTerminalType)));
			printf("src=%d\n", iot[i].d.ot->v1.bSourceId);
			break;
		case UDESCSUB_AC_MIXER:
			printf("AC_MIXER src=");
			for (j = 0; j < iot[i].d.mu->bNrInPins; j++)
				printf("%d ", iot[i].d.mu->baSourceId[j]);
			printf("\n\t");
			cluster = uaudio_get_cluster(sc, i, iot);
			uaudio_dump_cluster(sc, &cluster);
			printf("\n");
			break;
		case UDESCSUB_AC_SELECTOR:
			printf("AC_SELECTOR src=");
			for (j = 0; j < iot[i].d.su->bNrInPins; j++)
				printf("%d ", iot[i].d.su->baSourceId[j]);
			printf("\n");
			break;
		case UDESCSUB_AC_FEATURE:
			switch (sc->sc_version) {
			case UAUDIO_VERSION1:
				printf("AC_FEATURE src=%d\n", iot[i].d.fu->v1.bSourceId);
				break;
			case UAUDIO_VERSION2:
				printf("AC_FEATURE src=%d\n", iot[i].d.fu->v2.bSourceId);
				break;
			}
			break;
		case UDESCSUB_AC_EFFECT:
			switch (sc->sc_version) {
			case UAUDIO_VERSION1:
				printf("AC_EFFECT src=%d\n", iot[i].d.fu->v1.bSourceId);
				break;
			case UAUDIO_VERSION2:
				printf("AC_EFFECT src=%d\n", iot[i].d.fu->v2.bSourceId);
				break;
			}
			break;
		case UDESCSUB_AC_PROCESSING:
			printf("AC_PROCESSING src=");
			for (j = 0; j < iot[i].d.pu->bNrInPins; j++)
				printf("%d ", iot[i].d.pu->baSourceId[j]);
			printf("\n\t");
			cluster = uaudio_get_cluster(sc, i, iot);
			uaudio_dump_cluster(sc, &cluster);
			printf("\n");
			break;
		case UDESCSUB_AC_EXTENSION:
			printf("AC_EXTENSION src=");
			for (j = 0; j < iot[i].d.eu->bNrInPins; j++)
				printf("%d ", iot[i].d.eu->baSourceId[j]);
			printf("\n\t");
			cluster = uaudio_get_cluster(sc, i, iot);
			uaudio_dump_cluster(sc, &cluster);
			printf("\n");
			break;
		case UDESCSUB_AC_CLKSRC:
			printf("AC_CLKSRC src=%d\n", iot[i].d.cu->iClockSource);
			break;
		case UDESCSUB_AC_CLKSEL:
			printf("AC_CLKSEL src=");
			for (j = 0; j < iot[i].d.su->bNrInPins; j++)
				printf("%d ", iot[i].d.su->baSourceId[j]);
			printf("\n");
			break;
		case UDESCSUB_AC_CLKMULT:
			printf("AC_CLKMULT not supported\n");
			break;
		case UDESCSUB_AC_RATECONV:
			printf("AC_RATEVONC not supported\n");
			break;
		default:
			printf("unknown audio control (subtype=%d)\n",
				  iot[i].d.desc->bDescriptorSubtype);
		}
		for (j = 0; j < iot[i].inputs_size; j++) {
			printf("\tinput%d: ", j);
			uaudio_dump_tml(iot[i].inputs[j]);
		}
		printf("\toutput: ");
		uaudio_dump_tml(iot[i].output);
	}
#endif

	sc->sc_nratectls = 0;
	for (i = 0; i < ndps; i++) {
		dp = iot[i].d.desc;
		if (dp == NULL)
			continue;
		DPRINTF("id=%d subtype=%d\n", i, dp->bDescriptorSubtype);
		switch (dp->bDescriptorSubtype) {
		case UDESCSUB_AC_HEADER:
			aprint_error("uaudio_identify_ac: unexpected AC header\n");
			break;
		case UDESCSUB_AC_INPUT:
			uaudio_add_input(sc, iot, i);
			break;
		case UDESCSUB_AC_OUTPUT:
			uaudio_add_output(sc, iot, i);
			break;
		case UDESCSUB_AC_MIXER:
			uaudio_add_mixer(sc, iot, i);
			break;
		case UDESCSUB_AC_SELECTOR:
			uaudio_add_selector(sc, iot, i);
			break;
		case UDESCSUB_AC_FEATURE:
			uaudio_add_feature(sc, iot, i);
			break;
		case UDESCSUB_AC_EFFECT:
			uaudio_add_effect(sc, iot, i);
			break;
		case UDESCSUB_AC_PROCESSING:
			uaudio_add_processing(sc, iot, i);
			break;
		case UDESCSUB_AC_EXTENSION:
			uaudio_add_extension(sc, iot, i);
			break;
		case UDESCSUB_AC_CLKSRC:
			uaudio_add_clksrc(sc, iot, i);
			/* record ids of clock sources */
			if (sc->sc_nratectls < AUFMT_MAX_FREQUENCIES)
				sc->sc_ratectls[sc->sc_nratectls++] = sc->sc_nctls - 1;
			break;
		case UDESCSUB_AC_CLKSEL:
			uaudio_add_clksel(sc, iot, i);
			break;
		case UDESCSUB_AC_CLKMULT:
			/* not yet */
			break;
		case UDESCSUB_AC_RATECONV:
			/* not yet */
			break;
		default:
			aprint_error(
			    "uaudio_identify_ac: bad AC desc subtype=0x%02x\n",
			    dp->bDescriptorSubtype);
			break;
		}
	}

	switch (sc->sc_version) {
	case UAUDIO_VERSION2:
		/*
		 * UAC2 has separate rate controls which effectively creates
		 * a set of audio_formats per input and output and their
		 * associated clock sources.
		 *
		 * audio(4) can only handle audio_formats per direction.
		 * - ignore stream terminals
		 * - mark rates for record or play if associated with an input
		 *   or output terminal respectively.
		 */
		for (j = 0; j < sc->sc_nratectls; ++j) {
			uint16_t wi = sc->sc_ctls[sc->sc_ratectls[j]].wIndex;
			sc->sc_ratemode[j] = 0;
			for (i = 0; i < ndps; i++) {
				dp = iot[i].d.desc;
				if (dp == NULL)
					continue;
				switch (dp->bDescriptorSubtype) {
				case UDESCSUB_AC_INPUT:
					if (UGETW(iot[i].d.it->v2.wTerminalType) != UAT_STREAM &&
					    wi == MAKE(iot[i].d.it->v2.bCSourceId, sc->sc_ac_iface)) {
						sc->sc_ratemode[j] |= AUMODE_RECORD;
					}
					break;
				case UDESCSUB_AC_OUTPUT:
					if (UGETW(iot[i].d.it->v2.wTerminalType) != UAT_STREAM &&
					    wi == MAKE(iot[i].d.ot->v2.bCSourceId, sc->sc_ac_iface)) {
						sc->sc_ratemode[j] |= AUMODE_PLAY;
					}
					break;
				}
			}
		}
		break;
	}

	/* delete io_terminal */
	for (i = 0; i < 256; i++) {
		if (iot[i].d.desc == NULL)
			continue;
		if (iot[i].inputs != NULL) {
			for (j = 0; j < iot[i].inputs_size; j++) {
				if (iot[i].inputs[j] != NULL)
					free(iot[i].inputs[j], M_TEMP);
			}
			free(iot[i].inputs, M_TEMP);
		}
		if (iot[i].output != NULL)
			free(iot[i].output, M_TEMP);
		iot[i].d.desc = NULL;
	}
	free(iot, M_TEMP);

	return USBD_NORMAL_COMPLETION;
}

Static int
uaudio_query_devinfo(void *addr, mixer_devinfo_t *mi)
{
	struct uaudio_softc *sc;
	struct mixerctl *mc;
	int n, nctls, i;

	DPRINTFN(7, "index=%d\n", mi->index);
	sc = addr;
	if (sc->sc_dying)
		return EIO;

	n = mi->index;
	nctls = sc->sc_nctls;

	switch (n) {
	case UAC_OUTPUT:
		mi->type = AUDIO_MIXER_CLASS;
		mi->mixer_class = UAC_OUTPUT;
		mi->next = mi->prev = AUDIO_MIXER_LAST;
		strlcpy(mi->label.name, AudioCoutputs, sizeof(mi->label.name));
		return 0;
	case UAC_INPUT:
		mi->type = AUDIO_MIXER_CLASS;
		mi->mixer_class = UAC_INPUT;
		mi->next = mi->prev = AUDIO_MIXER_LAST;
		strlcpy(mi->label.name, AudioCinputs, sizeof(mi->label.name));
		return 0;
	case UAC_EQUAL:
		mi->type = AUDIO_MIXER_CLASS;
		mi->mixer_class = UAC_EQUAL;
		mi->next = mi->prev = AUDIO_MIXER_LAST;
		strlcpy(mi->label.name, AudioCequalization,
		    sizeof(mi->label.name));
		return 0;
	case UAC_RECORD:
		mi->type = AUDIO_MIXER_CLASS;
		mi->mixer_class = UAC_RECORD;
		mi->next = mi->prev = AUDIO_MIXER_LAST;
		strlcpy(mi->label.name, AudioCrecord, sizeof(mi->label.name));
		return 0;
	default:
		break;
	}

	n -= UAC_NCLASSES;
	if (n < 0 || n >= nctls)
		return ENXIO;

	mc = &sc->sc_ctls[n];
	strlcpy(mi->label.name, mc->ctlname, sizeof(mi->label.name));
	mi->mixer_class = mc->class;
	mi->next = mi->prev = AUDIO_MIXER_LAST;	/* XXX */
	switch (mc->type) {
	case MIX_ON_OFF:
		mi->type = AUDIO_MIXER_ENUM;
		mi->un.e.num_mem = 2;
		strlcpy(mi->un.e.member[0].label.name, AudioNoff,
		    sizeof(mi->un.e.member[0].label.name));
		mi->un.e.member[0].ord = 0;
		strlcpy(mi->un.e.member[1].label.name, AudioNon,
		    sizeof(mi->un.e.member[1].label.name));
		mi->un.e.member[1].ord = 1;
		break;
	case MIX_SELECTOR:
		n = uimin(mc->ranges[0].maxval - mc->ranges[0].minval + 1,
		    __arraycount(mi->un.e.member));
		mi->type = AUDIO_MIXER_ENUM;
		mi->un.e.num_mem = n;
		for (i = 0; i < n; i++) {
			snprintf(mi->un.e.member[i].label.name,
				 sizeof(mi->un.e.member[i].label.name),
				 "%d", i + mc->ranges[0].minval);
			mi->un.e.member[i].ord = i + mc->ranges[0].minval;
		}
		break;
	default:
		mi->type = AUDIO_MIXER_VALUE;
		strncpy(mi->un.v.units.name, mc->ctlunit, MAX_AUDIO_DEV_LEN);
		mi->un.v.num_channels = mc->nchan;
		mi->un.v.delta = mc->delta;
		break;
	}
	return 0;
}

Static int
uaudio_open(void *addr, int flags)
{
	struct uaudio_softc *sc;

	sc = addr;
	DPRINTF("sc=%p\n", sc);
	if (sc->sc_dying)
		return EIO;

	if ((flags & FWRITE) && !(sc->sc_mode & AUMODE_PLAY))
		return EACCES;
	if ((flags & FREAD) && !(sc->sc_mode & AUMODE_RECORD))
		return EACCES;

	return 0;
}

Static int
uaudio_halt_out_dma(void *addr)
{
	struct uaudio_softc *sc = addr;

	DPRINTF("%s", "enter\n");

	mutex_exit(&sc->sc_intr_lock);
	uaudio_halt_out_dma_unlocked(sc);
	mutex_enter(&sc->sc_intr_lock);

	return 0;
}

Static void
uaudio_halt_out_dma_unlocked(struct uaudio_softc *sc)
{
	if (sc->sc_playchan.pipe != NULL) {
		uaudio_chan_abort(sc, &sc->sc_playchan);
		uaudio_chan_free_buffers(sc, &sc->sc_playchan);
		uaudio_chan_close(sc, &sc->sc_playchan);
		sc->sc_playchan.intr = NULL;
	}
}

Static int
uaudio_halt_in_dma(void *addr)
{
	struct uaudio_softc *sc = addr;

	DPRINTF("%s", "enter\n");

	mutex_exit(&sc->sc_intr_lock);
	uaudio_halt_in_dma_unlocked(sc);
	mutex_enter(&sc->sc_intr_lock);

	return 0;
}

Static void
uaudio_halt_in_dma_unlocked(struct uaudio_softc *sc)
{
	if (sc->sc_recchan.pipe != NULL) {
		uaudio_chan_abort(sc, &sc->sc_recchan);
		uaudio_chan_free_buffers(sc, &sc->sc_recchan);
		uaudio_chan_close(sc, &sc->sc_recchan);
		sc->sc_recchan.intr = NULL;
	}
}

Static int
uaudio_getdev(void *addr, struct audio_device *retp)
{
	struct uaudio_softc *sc;

	DPRINTF("%s", "\n");
	sc = addr;
	if (sc->sc_dying)
		return EIO;

	*retp = sc->sc_adev;
	return 0;
}

/*
 * Make sure the block size is large enough to hold all outstanding transfers.
 */
Static int
uaudio_round_blocksize(void *addr, int blk,
		       int mode, const audio_params_t *param)
{
	struct uaudio_softc *sc;
	int b;

	sc = addr;
	DPRINTF("blk=%d mode=%s\n", blk,
	    mode == AUMODE_PLAY ? "AUMODE_PLAY" : "AUMODE_RECORD");

	/* chan.bytes_per_frame can be 0. */
	if (mode == AUMODE_PLAY || sc->sc_recchan.bytes_per_frame <= 0) {
		b = param->sample_rate * sc->sc_recchan.nframes
		    * sc->sc_recchan.nchanbufs;

		/*
		 * This does not make accurate value in the case
		 * of b % usb_frames_per_second != 0
		 */
		b /= sc->sc_usb_frames_per_second;

		b *= param->precision / 8 * param->channels;
	} else {
		/*
		 * use wMaxPacketSize in bytes_per_frame.
		 * See uaudio_set_format() and uaudio_chan_init()
		 */
		b = sc->sc_recchan.bytes_per_frame
		    * sc->sc_recchan.nframes * sc->sc_recchan.nchanbufs;
	}

	if (b <= 0)
		b = 1;
	blk = blk <= b ? b : blk / b * b;

#ifdef DIAGNOSTIC
	if (blk <= 0) {
		aprint_debug("uaudio_round_blocksize: blk=%d\n", blk);
		blk = 512;
	}
#endif

	DPRINTF("resultant blk=%d\n", blk);
	return blk;
}

Static int
uaudio_get_props(void *addr)
{
	struct uaudio_softc *sc;
	int props;

	sc = addr;
	props = 0;
	if ((sc->sc_mode & AUMODE_PLAY))
		props |= AUDIO_PROP_PLAYBACK;
	if ((sc->sc_mode & AUMODE_RECORD))
		props |= AUDIO_PROP_CAPTURE;

	/* XXX I'm not sure all bidirectional devices support FULLDUP&INDEP */
	if (props == (AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE))
		props |= AUDIO_PROP_FULLDUPLEX | AUDIO_PROP_INDEPENDENT;

	return props;
}

Static void
uaudio_get_locks(void *addr, kmutex_t **intr, kmutex_t **thread)
{
	struct uaudio_softc *sc;

	sc = addr;
	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

Static int
uaudio_get(struct uaudio_softc *sc, int which, int type, int wValue,
	   int wIndex, int len)
{
	usb_device_request_t req;
	uint8_t data[4];
	usbd_status err;
	int val;

	if (wValue == -1)
		return 0;

	req.bmRequestType = type;
	req.bRequest = which;
	USETW(req.wValue, wValue);
	USETW(req.wIndex, wIndex);
	USETW(req.wLength, len);
	DPRINTFN(2,"type=0x%02x req=0x%02x wValue=0x%04x "
		    "wIndex=0x%04x len=%d\n",
		    type, which, wValue, wIndex, len);
	err = usbd_do_request(sc->sc_udev, &req, data);
	if (err) {
		DPRINTF("err=%s\n", usbd_errstr(err));
		return -1;
	}
	switch (len) {
	case 1:
		val = data[0];
		break;
	case 2:
		val = data[0];
		val |= data[1] << 8;
		break;
	case 3:
		val = data[0];
		val |= data[1] << 8;
		val |= data[2] << 16;
		break;
	case 4:
		val = data[0];
		val |= data[1] << 8;
		val |= data[2] << 16;
		val |= data[3] << 24;
		break;
	default:
		DPRINTF("bad length=%d\n", len);
		return -1;
	}
	DPRINTFN(2,"val=%d\n", val);
	return val;
}

Static int
uaudio_getbuf(struct uaudio_softc *sc, int which, int type, int wValue,
	   int wIndex, int len, uint8_t *data)
{
	usb_device_request_t req;
	usbd_status err;

	req.bmRequestType = type;
	req.bRequest = which;
	USETW(req.wValue, wValue);
	USETW(req.wIndex, wIndex);
	USETW(req.wLength, len);
	DPRINTFN(2,"type=0x%02x req=0x%02x wValue=0x%04x "
		    "wIndex=0x%04x len=%d\n",
		    type, which, wValue, wIndex, len);
	err = usbd_do_request(sc->sc_udev, &req, data);
	if (err) {
		DPRINTF("err=%s\n", usbd_errstr(err));
		return -1;
	}

	DPRINTFN(2,"val@%p\n", data);
	return 0;
}

Static void
uaudio_set(struct uaudio_softc *sc, int which, int type, int wValue,
	   int wIndex, int len, int val)
{
	usb_device_request_t req;
	uint8_t data[4];
	int err __unused;

	if (wValue == -1)
		return;

	req.bmRequestType = type;
	req.bRequest = which;
	USETW(req.wValue, wValue);
	USETW(req.wIndex, wIndex);
	USETW(req.wLength, len);

	data[0] = val;
	data[1] = val >> 8;
	data[2] = val >> 16;
	data[3] = val >> 24;

	DPRINTFN(2,"type=0x%02x req=0x%02x wValue=0x%04x "
		    "wIndex=0x%04x len=%d, val=%d\n",
		    type, which, wValue, wIndex, len, val);
	err = usbd_do_request(sc->sc_udev, &req, data);
#ifdef UAUDIO_DEBUG
	if (err)
		DPRINTF("err=%s\n", usbd_errstr(err));
#endif
}

Static int
uaudio_signext(int type, int val)
{
	if (MIX_UNSIGNED(type)) {
		switch (MIX_SIZE(type)) {
		case 1:
			val = (uint8_t)val;
			break;
		case 2:
			val = (uint16_t)val;
			break;
		case 3:
			val = ((uint32_t)val << 8) >> 8;
			break;
		case 4:
			val = (uint32_t)val;
			break;
		}
	} else {
		switch (MIX_SIZE(type)) {
		case 1:
			val = (int8_t)val;
			break;
		case 2:
			val = (int16_t)val;
			break;
		case 3:
			val = ((int32_t)val << 8) >> 8;
			break;
		case 4:
			val = (int32_t)val;
			break;
		}
	}
	return val;
}

Static int
uaudio_value2bsd(struct mixerctl *mc, int val)
{
	DPRINTFN(5, "type=%03x val=%d min=%d max=%d ",
		     mc->type, val, mc->ranges[0].minval, mc->ranges[0].maxval);
	if (mc->type == MIX_ON_OFF) {
		val = (val != 0);
	} else if (mc->type == MIX_SELECTOR) {
		if (val < mc->ranges[0].minval)
			val = mc->ranges[0].minval;
		if (val > mc->ranges[0].maxval)
			val = mc->ranges[0].maxval;
	} else if (mc->mul > 0) {
		val = ((uaudio_signext(mc->type, val) - mc->ranges[0].minval)
		    * 255 + mc->mul - 1) / mc->mul;
	} else
		val = 0;
	DPRINTFN_CLEAN(5, "val'=%d\n", val);
	return val;
}

Static int
uaudio_bsd2value(struct mixerctl *mc, int val)
{
	int i;

	DPRINTFN(5,"type=%03x val=%d min=%d max=%d ",
		    mc->type, val, mc->ranges[0].minval, mc->ranges[0].maxval);
	if (mc->type == MIX_ON_OFF) {
		val = (val != 0);
	} else if (mc->type == MIX_SELECTOR) {
		if (val < mc->ranges[0].minval)
			val = mc->ranges[0].minval;
		if (val > mc->ranges[0].maxval)
			val = mc->ranges[0].maxval;
	} else {
		if (val < 0)
			val = 0;
		else if (val > 255)
			val = 255;

		val = val * (mc->mul + 1) / 256 + mc->ranges[0].minval;

		for (i=0; i<mc->nranges; ++i) {
			struct range *r = &mc->ranges[i];

			if (r->resval == 0)
				continue;
			if (val > r->maxval)
				continue;
			if (val < r->minval)
				val = r->minval;
			val = (val - r->minval + r->resval/2)
			    / r->resval * r->resval
			    + r->minval;
			break;
		}
	}
	DPRINTFN_CLEAN(5, "val'=%d\n", val);
	return val;
}

Static const char *
uaudio_clockname(u_int attr)
{
	static const char *names[] = {
		"clkext",
		"clkfixed",
		"clkvar",
		"clkprog"
	};

	return names[attr & 3];
}

Static int
uaudio_makename(struct uaudio_softc *sc, uByte idx, const char *defname, uByte id, char *buf, size_t len)
{
	char *tmp;
	int err, count;

	tmp = kmem_alloc(USB_MAX_ENCODED_STRING_LEN, KM_SLEEP);
	err = usbd_get_string0(sc->sc_udev, idx, tmp, true);

	if (id != 0 || err)
		count = snprintf(buf, len, "%s%d", err ? defname : tmp, id);
	else
		count = snprintf(buf, len, "%s", err ? defname : tmp);

	kmem_free(tmp, USB_MAX_ENCODED_STRING_LEN);

	return count;
}


Static int
uaudio_ctl_get(struct uaudio_softc *sc, int which, struct mixerctl *mc,
	       int chan)
{
	int val;

	DPRINTFN(5,"which=%d chan=%d ctl=%s type=%d\n", which, chan, mc->ctlname, mc->type);
	mutex_exit(&sc->sc_lock);
	val = uaudio_get(sc, which, UT_READ_CLASS_INTERFACE, mc->wValue[chan],
			 mc->wIndex, MIX_SIZE(mc->type));
	mutex_enter(&sc->sc_lock);
	return uaudio_value2bsd(mc, val);
}

Static void
uaudio_ctl_set(struct uaudio_softc *sc, int which, struct mixerctl *mc,
	       int chan, int val)
{

	DPRINTFN(5,"which=%d chan=%d ctl=%s type=%d\n", which, chan, mc->ctlname, mc->type);
	val = uaudio_bsd2value(mc, val);
	mutex_exit(&sc->sc_lock);
	uaudio_set(sc, which, UT_WRITE_CLASS_INTERFACE, mc->wValue[chan],
		   mc->wIndex, MIX_SIZE(mc->type), val);
	mutex_enter(&sc->sc_lock);
}

Static int
uaudio_mixer_get_port(void *addr, mixer_ctrl_t *cp)
{
	struct uaudio_softc *sc;
	struct mixerctl *mc;
	int i, n, vals[MIX_MAX_CHAN], val;
	int req;

	DPRINTFN(2, "index=%d\n", cp->dev);
	sc = addr;
	if (sc->sc_dying)
		return EIO;

	req = sc->sc_version == UAUDIO_VERSION2 ? V2_CUR : GET_CUR;

	n = cp->dev - UAC_NCLASSES;
	if (n < 0 || n >= sc->sc_nctls)
		return ENXIO;
	mc = &sc->sc_ctls[n];

	if (mc->type == MIX_ON_OFF) {
		if (cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;
		cp->un.ord = uaudio_ctl_get(sc, req, mc, 0);
	} else if (mc->type == MIX_SELECTOR) {
		if (cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;
		cp->un.ord = uaudio_ctl_get(sc, req, mc, 0);
	} else {
		if (cp->type != AUDIO_MIXER_VALUE)
			return EINVAL;
		if (cp->un.value.num_channels != 1 &&
		    cp->un.value.num_channels != mc->nchan)
			return EINVAL;
		for (i = 0; i < mc->nchan; i++)
			vals[i] = uaudio_ctl_get(sc, req, mc, i);
		if (cp->un.value.num_channels == 1 && mc->nchan != 1) {
			for (val = 0, i = 0; i < mc->nchan; i++)
				val += vals[i];
			vals[0] = val / mc->nchan;
		}
		for (i = 0; i < cp->un.value.num_channels; i++)
			cp->un.value.level[i] = vals[i];
	}

	return 0;
}

Static int
uaudio_mixer_set_port(void *addr, mixer_ctrl_t *cp)
{
	struct uaudio_softc *sc;
	struct mixerctl *mc;
	int i, n, vals[MIX_MAX_CHAN];
	int req;

	DPRINTFN(2, "index = %d\n", cp->dev);
	sc = addr;
	if (sc->sc_dying)
		return EIO;

	req = sc->sc_version == UAUDIO_VERSION2 ? V2_CUR : SET_CUR;

	n = cp->dev - UAC_NCLASSES;
	if (n < 0 || n >= sc->sc_nctls)
		return ENXIO;
	mc = &sc->sc_ctls[n];

	if (mc->type == MIX_ON_OFF) {
		if (cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;
		uaudio_ctl_set(sc, req, mc, 0, cp->un.ord);
	} else if (mc->type == MIX_SELECTOR) {
		if (cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;
		uaudio_ctl_set(sc, req, mc, 0, cp->un.ord);
	} else {
		if (cp->type != AUDIO_MIXER_VALUE)
			return EINVAL;
		if (cp->un.value.num_channels == 1)
			for (i = 0; i < mc->nchan; i++)
				vals[i] = cp->un.value.level[0];
		else if (cp->un.value.num_channels == mc->nchan)
			for (i = 0; i < mc->nchan; i++)
				vals[i] = cp->un.value.level[i];
		else
			return EINVAL;
		for (i = 0; i < mc->nchan; i++)
			uaudio_ctl_set(sc, req, mc, i, vals[i]);
	}
	return 0;
}

Static int
uaudio_trigger_input(void *addr, void *start, void *end, int blksize,
		     void (*intr)(void *), void *arg,
		     const audio_params_t *param)
{
	struct uaudio_softc *sc;
	struct chan *ch;
	usbd_status err;
	int i;

	sc = addr;
	if (sc->sc_dying)
		return EIO;

	mutex_exit(&sc->sc_intr_lock);

	DPRINTFN(3, "sc=%p start=%p end=%p "
		    "blksize=%d\n", sc, start, end, blksize);
	ch = &sc->sc_recchan;
	uaudio_chan_set_param(ch, start, end, blksize);
	DPRINTFN(3, "sample_size=%d bytes/frame=%d "
		    "fraction=0.%03d\n", ch->sample_size, ch->bytes_per_frame,
		    ch->fraction);

	err = uaudio_chan_open(sc, ch);
	if (err) {
		mutex_enter(&sc->sc_intr_lock);
		device_printf(sc->sc_dev,"%s open channel err=%s\n",__func__, usbd_errstr(err));
		return EIO;
	}

	err = uaudio_chan_alloc_buffers(sc, ch);
	if (err) {
		uaudio_chan_close(sc, ch);
		device_printf(sc->sc_dev,"%s alloc buffers err=%s\n",__func__, usbd_errstr(err));
		mutex_enter(&sc->sc_intr_lock);
		return EIO;
	}


	ch->intr = intr;
	ch->arg = arg;

	/*
	 * Start as half as many channels for recording as for playback.
	 * This stops playback from stuttering in full-duplex operation.
	 */
	for (i = 0; i < ch->nchanbufs / 2; i++) {
		uaudio_chan_rtransfer(ch);
	}

	mutex_enter(&sc->sc_intr_lock);

	return 0;
}

Static int
uaudio_trigger_output(void *addr, void *start, void *end, int blksize,
		      void (*intr)(void *), void *arg,
		      const audio_params_t *param)
{
	struct uaudio_softc *sc;
	struct chan *ch;
	usbd_status err;
	int i;

	sc = addr;
	if (sc->sc_dying)
		return EIO;

	mutex_exit(&sc->sc_intr_lock);

	DPRINTFN(3, "sc=%p start=%p end=%p "
		    "blksize=%d\n", sc, start, end, blksize);
	ch = &sc->sc_playchan;
	uaudio_chan_set_param(ch, start, end, blksize);
	DPRINTFN(3, "sample_size=%d bytes/frame=%d "
		    "fraction=0.%03d\n", ch->sample_size, ch->bytes_per_frame,
		    ch->fraction);

	err = uaudio_chan_open(sc, ch);
	if (err) {
		mutex_enter(&sc->sc_intr_lock);
		device_printf(sc->sc_dev,"%s open channel err=%s\n",__func__, usbd_errstr(err));
		return EIO;
	}

	err = uaudio_chan_alloc_buffers(sc, ch);
	if (err) {
		uaudio_chan_close(sc, ch);
		device_printf(sc->sc_dev,"%s alloc buffers err=%s\n",__func__, usbd_errstr(err));
		mutex_enter(&sc->sc_intr_lock);
		return EIO;
	}

	ch->intr = intr;
	ch->arg = arg;

	for (i = 0; i < ch->nchanbufs; i++)
		uaudio_chan_ptransfer(ch);

	mutex_enter(&sc->sc_intr_lock);

	return 0;
}

/* Set up a pipe for a channel. */
Static usbd_status
uaudio_chan_open(struct uaudio_softc *sc, struct chan *ch)
{
	struct as_info *as;
	usb_device_descriptor_t *ddesc;
	int endpt, clkid;
	usbd_status err;

	as = &sc->sc_alts[ch->altidx];
	endpt = as->edesc->bEndpointAddress;
	clkid = sc->sc_clock[as->terminal];
	DPRINTF("endpt=0x%02x, clkid=%d, speed=%d, alt=%d\n",
		 endpt, clkid, ch->sample_rate, as->alt);

	/* Set alternate interface corresponding to the mode. */
	err = usbd_set_interface(as->ifaceh, as->alt);
	if (err)
		return err;

	/*
	 * Roland SD-90 freezes by a SAMPLING_FREQ_CONTROL request.
	 */
	ddesc = usbd_get_device_descriptor(sc->sc_udev);
	if ((UGETW(ddesc->idVendor) != USB_VENDOR_ROLAND) &&
	    (UGETW(ddesc->idProduct) != USB_PRODUCT_ROLAND_SD90)) {
		err = uaudio_set_speed(sc, endpt, clkid, ch->sample_rate);
		if (err) {
			DPRINTF("set_speed failed err=%s\n", usbd_errstr(err));
		}
	}

	DPRINTF("create pipe to 0x%02x\n", endpt);
	err = usbd_open_pipe(as->ifaceh, endpt, USBD_MPSAFE, &ch->pipe);
	if (err)
		return err;
	if (as->edesc1 != NULL) {
		endpt = as->edesc1->bEndpointAddress;
		if (endpt != 0) {
			DPRINTF("create sync-pipe to 0x%02x\n", endpt);
			err = usbd_open_pipe(as->ifaceh, endpt, USBD_MPSAFE,
			    &ch->sync_pipe);
		}
	}

	return err;
}

Static void
uaudio_chan_abort(struct uaudio_softc *sc, struct chan *ch)
{
	struct usbd_pipe *pipe;
	struct as_info *as;

	as = &sc->sc_alts[ch->altidx];
	as->sc_busy = 0;
	if (sc->sc_nullalt >= 0) {
		DPRINTF("set null alt=%d\n", sc->sc_nullalt);
		usbd_set_interface(as->ifaceh, sc->sc_nullalt);
	}
	pipe = ch->pipe;
	if (pipe) {
		usbd_abort_pipe(pipe);
	}
	pipe = ch->sync_pipe;
	if (pipe) {
		usbd_abort_pipe(pipe);
	}
}

Static void
uaudio_chan_close(struct uaudio_softc *sc, struct chan *ch)
{
	struct usbd_pipe *pipe;

	pipe = atomic_swap_ptr(&ch->pipe, NULL);
	if (pipe) {
		usbd_close_pipe(pipe);
	}
	pipe = atomic_swap_ptr(&ch->sync_pipe, NULL);
	if (pipe) {
		usbd_close_pipe(pipe);
	}
}

Static usbd_status
uaudio_chan_alloc_buffers(struct uaudio_softc *sc, struct chan *ch)
{
	int i, size;

	size = (ch->bytes_per_frame + ch->sample_size) * ch->nframes;
	for (i = 0; i < ch->nchanbufs; i++) {
		struct usbd_xfer *xfer;

		int err = usbd_create_xfer(ch->pipe, size, 0, ch->nframes,
		    &xfer);
		if (err)
			goto bad;

		ch->chanbufs[i].xfer = xfer;
		ch->chanbufs[i].buffer = usbd_get_buffer(xfer);
		ch->chanbufs[i].chan = ch;
	}

	return USBD_NORMAL_COMPLETION;

bad:
	while (--i >= 0)
		/* implicit buffer free */
		usbd_destroy_xfer(ch->chanbufs[i].xfer);
	return USBD_NOMEM;
}

Static void
uaudio_chan_free_buffers(struct uaudio_softc *sc, struct chan *ch)
{
	int i;

	for (i = 0; i < ch->nchanbufs; i++)
		usbd_destroy_xfer(ch->chanbufs[i].xfer);
}

Static void
uaudio_chan_ptransfer(struct chan *ch)
{
	struct uaudio_softc *sc = ch->sc;
	struct chanbuf *cb;
	int i, n, size, residue, total;

	if (sc->sc_dying)
		return;

	/* Pick the next channel buffer. */
	cb = &ch->chanbufs[ch->curchanbuf];
	if (++ch->curchanbuf >= ch->nchanbufs)
		ch->curchanbuf = 0;

	/* Compute the size of each frame in the next transfer. */
	residue = ch->residue;
	total = 0;
	for (i = 0; i < ch->nframes; i++) {
		size = ch->bytes_per_frame;
		residue += ch->fraction;
		if (residue >= sc->sc_usb_frames_per_second) {
			if ((sc->sc_altflags & UA_NOFRAC) == 0)
				size += ch->sample_size;
			residue -= sc->sc_usb_frames_per_second;
		}
		cb->sizes[i] = size;
		total += size;
	}
	ch->residue = residue;
	cb->size = total;

	/*
	 * Transfer data from upper layer buffer to channel buffer, taking
	 * care of wrapping the upper layer buffer.
	 */
	n = uimin(total, ch->end - ch->cur);
	memcpy(cb->buffer, ch->cur, n);
	ch->cur += n;
	if (ch->cur >= ch->end)
		ch->cur = ch->start;
	if (total > n) {
		total -= n;
		memcpy(cb->buffer + n, ch->cur, total);
		ch->cur += total;
	}

#ifdef UAUDIO_DEBUG
	if (uaudiodebug > 8) {
		DPRINTF("buffer=%p, residue=0.%03d\n", cb->buffer, ch->residue);
		for (i = 0; i < ch->nframes; i++) {
			DPRINTF("   [%d] length %d\n", i, cb->sizes[i]);
		}
	}
#endif

	//DPRINTFN(5, "ptransfer xfer=%p\n", cb->xfer);
	/* Fill the request */
	usbd_setup_isoc_xfer(cb->xfer, cb, cb->sizes, ch->nframes, 0,
	    uaudio_chan_pintr);

	usbd_status err = usbd_transfer(cb->xfer);
	if (err != USBD_IN_PROGRESS && err != USBD_NORMAL_COMPLETION)
		device_printf(sc->sc_dev, "ptransfer error %d\n", err);
}

Static void
uaudio_chan_pintr(struct usbd_xfer *xfer, void *priv,
		  usbd_status status)
{
	struct uaudio_softc *sc;
	struct chanbuf *cb;
	struct chan *ch;
	uint32_t count;

	cb = priv;
	ch = cb->chan;
	sc = ch->sc;
	/* Return if we are aborting. */
	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION)
		device_printf(sc->sc_dev, "pintr error: %s\n",
		              usbd_errstr(status));

	usbd_get_xfer_status(xfer, NULL, NULL, &count, NULL);
	DPRINTFN(5, "count=%d, transferred=%d\n",
		    count, ch->transferred);
#ifdef DIAGNOSTIC
	if (count != cb->size) {
		device_printf(sc->sc_dev,
		    "uaudio_chan_pintr: count(%d) != size(%d), status(%d)\n",
		    count, cb->size, status);
	}
#endif

	mutex_enter(&sc->sc_intr_lock);
	ch->transferred += cb->size;
	/* Call back to upper layer */
	while (ch->transferred >= ch->blksize) {
		ch->transferred -= ch->blksize;
		DPRINTFN(5, "call %p(%p)\n", ch->intr, ch->arg);
		ch->intr(ch->arg);
	}
	mutex_exit(&sc->sc_intr_lock);

	/* start next transfer */
	uaudio_chan_ptransfer(ch);
}

Static void
uaudio_chan_rtransfer(struct chan *ch)
{
	struct uaudio_softc *sc = ch->sc;
	struct chanbuf *cb;
	int i, size, residue, total;

	if (sc->sc_dying)
		return;

	/* Pick the next channel buffer. */
	cb = &ch->chanbufs[ch->curchanbuf];
	if (++ch->curchanbuf >= ch->nchanbufs)
		ch->curchanbuf = 0;

	/* Compute the size of each frame in the next transfer. */
	residue = ch->residue;
	total = 0;
	for (i = 0; i < ch->nframes; i++) {
		size = ch->bytes_per_frame;
#if 0
		residue += ch->fraction;
		if (residue >= sc->sc_usb_frames_per_second) {
			if ((sc->sc_altflags & UA_NOFRAC) == 0)
				size += ch->sample_size;
			residue -= sc->sc_usb_frames_per_second;
		}
#endif
		cb->sizes[i] = size;
		cb->offsets[i] = total;
		total += size;
	}
	ch->residue = residue;
	cb->size = total;

#ifdef UAUDIO_DEBUG
	if (uaudiodebug > 8) {
		DPRINTF("buffer=%p, residue=0.%03d\n", cb->buffer, ch->residue);
		for (i = 0; i < ch->nframes; i++) {
			DPRINTF("   [%d] length %d\n", i, cb->sizes[i]);
		}
	}
#endif

	DPRINTFN(5, "transfer xfer=%p\n", cb->xfer);
	/* Fill the request */
	usbd_setup_isoc_xfer(cb->xfer, cb, cb->sizes, ch->nframes, 0,
	    uaudio_chan_rintr);

	usbd_status err = usbd_transfer(cb->xfer);
	if (err != USBD_IN_PROGRESS && err != USBD_NORMAL_COMPLETION)
		device_printf(sc->sc_dev, "rtransfer error %d\n", err);
}

Static void
uaudio_chan_rintr(struct usbd_xfer *xfer, void *priv,
		  usbd_status status)
{
	struct uaudio_softc *sc;
	struct chanbuf *cb;
	struct chan *ch;
	uint32_t count;
	int i, n, frsize;

	cb = priv;
	ch = cb->chan;
	sc = ch->sc;
	/* Return if we are aborting. */
	if (status == USBD_CANCELLED)
		return;

	if (status != USBD_NORMAL_COMPLETION && status != USBD_SHORT_XFER)
		device_printf(sc->sc_dev, "rintr error: %s\n",
		              usbd_errstr(status));

	usbd_get_xfer_status(xfer, NULL, NULL, &count, NULL);
	DPRINTFN(5, "count=%d, transferred=%d\n", count, ch->transferred);

	/* count < cb->size is normal for asynchronous source */
#ifdef DIAGNOSTIC
	if (count > cb->size) {
		device_printf(sc->sc_dev,
		    "uaudio_chan_rintr: count(%d) > size(%d) status(%d)\n",
		    count, cb->size, status);
	}
#endif

	/*
	 * Transfer data from channel buffer to upper layer buffer, taking
	 * care of wrapping the upper layer buffer.
	 */
	for (i = 0; i < ch->nframes; i++) {
		frsize = cb->sizes[i];
		n = uimin(frsize, ch->end - ch->cur);
		memcpy(ch->cur, cb->buffer + cb->offsets[i], n);
		ch->cur += n;
		if (ch->cur >= ch->end)
			ch->cur = ch->start;
		if (frsize > n) {
			memcpy(ch->cur, cb->buffer + cb->offsets[i] + n,
			    frsize - n);
			ch->cur += frsize - n;
		}
	}

	/* Call back to upper layer */
	mutex_enter(&sc->sc_intr_lock);
	ch->transferred += count;
	while (ch->transferred >= ch->blksize) {
		ch->transferred -= ch->blksize;
		DPRINTFN(5, "call %p(%p)\n", ch->intr, ch->arg);
		ch->intr(ch->arg);
	}
	mutex_exit(&sc->sc_intr_lock);

	/* start next transfer */
	uaudio_chan_rtransfer(ch);
}

Static void
uaudio_chan_init(struct chan *ch, int altidx,
    const struct audio_params *param, int maxpktsize, bool isrecord)
{
	struct uaudio_softc *sc = ch->sc;
	int samples_per_frame, sample_size;

	DPRINTFN(5, "altidx=%d, %d/%d %dch %dHz ufps %u max %d\n",
		altidx, param->validbits, param->precision, param->channels,
		param->sample_rate, sc->sc_usb_frames_per_second, maxpktsize);

	ch->altidx = altidx;
	sample_size = param->precision * param->channels / 8;

	if (isrecord) {
		if (maxpktsize >= sample_size)
			samples_per_frame = maxpktsize / sample_size;
		else
			samples_per_frame = param->sample_rate / sc->sc_usb_frames_per_second
			    + param->channels;
		ch->fraction = 0;
	} else {
		samples_per_frame = param->sample_rate / sc->sc_usb_frames_per_second;
		ch->fraction = param->sample_rate % sc->sc_usb_frames_per_second;
	}

	ch->sample_size = sample_size;
	ch->sample_rate = param->sample_rate;
	ch->bytes_per_frame = samples_per_frame * sample_size;

	if (maxpktsize > 0 && ch->bytes_per_frame > maxpktsize) {
		samples_per_frame = maxpktsize / sample_size;
		ch->bytes_per_frame = samples_per_frame * sample_size;
	}

	ch->residue = 0;
}

Static void
uaudio_chan_set_param(struct chan *ch, u_char *start, u_char *end, int blksize)
{

	ch->start = start;
	ch->end = end;
	ch->cur = start;
	ch->blksize = blksize;
	ch->transferred = 0;
	ch->curchanbuf = 0;
}

Static int
uaudio_set_format(void *addr, int setmode,
		  const audio_params_t *play, const audio_params_t *rec,
		  audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct uaudio_softc *sc;
	int paltidx, raltidx;

	sc = addr;
	paltidx = -1;
	raltidx = -1;
	if (sc->sc_dying)
		return EIO;

	if ((setmode & AUMODE_PLAY) && sc->sc_playchan.altidx != -1) {
		sc->sc_alts[sc->sc_playchan.altidx].sc_busy = 0;
	}
	if ((setmode & AUMODE_RECORD) && sc->sc_recchan.altidx != -1) {
		sc->sc_alts[sc->sc_recchan.altidx].sc_busy = 0;
	}

	/* Some uaudio devices are unidirectional.  Don't try to find a
	   matching mode for the unsupported direction. */
	setmode &= sc->sc_mode;

	if ((setmode & AUMODE_PLAY)) {
		paltidx = audio_indexof_format(sc->sc_formats, sc->sc_nformats,
		    AUMODE_PLAY, play);
		/* Transfer should have halted */
		uaudio_chan_init(&sc->sc_playchan, paltidx, play,
		    UGETW(sc->sc_alts[paltidx].edesc->wMaxPacketSize), false);
	}
	if ((setmode & AUMODE_RECORD)) {
		raltidx = audio_indexof_format(sc->sc_formats, sc->sc_nformats,
		    AUMODE_RECORD, rec);
		/* Transfer should have halted */
		uaudio_chan_init(&sc->sc_recchan, raltidx, rec,
		    UGETW(sc->sc_alts[raltidx].edesc->wMaxPacketSize), true);
	}

	if ((setmode & AUMODE_PLAY) && sc->sc_playchan.altidx != -1) {
		sc->sc_alts[sc->sc_playchan.altidx].sc_busy = 1;
	}
	if ((setmode & AUMODE_RECORD) && sc->sc_recchan.altidx != -1) {
		sc->sc_alts[sc->sc_recchan.altidx].sc_busy = 1;
	}

	DPRINTF("use altidx=p%d/r%d, altno=p%d/r%d\n",
		 sc->sc_playchan.altidx, sc->sc_recchan.altidx,
		 (sc->sc_playchan.altidx >= 0)
		   ?sc->sc_alts[sc->sc_playchan.altidx].idesc->bAlternateSetting
		   : -1,
		 (sc->sc_recchan.altidx >= 0)
		   ? sc->sc_alts[sc->sc_recchan.altidx].idesc->bAlternateSetting
		   : -1);

	return 0;
}

Static usbd_status
uaudio_speed(struct uaudio_softc *sc, int endpt, int clkid,
    uint8_t *data, int set)
{
	usb_device_request_t req;

	switch (sc->sc_version) {
	case UAUDIO_VERSION1:
		req.bmRequestType = set ?
			UT_WRITE_CLASS_ENDPOINT
			: UT_READ_CLASS_ENDPOINT;
		req.bRequest = set ?
			SET_CUR
			: GET_CUR;
		USETW2(req.wValue, SAMPLING_FREQ_CONTROL, 0);
		USETW(req.wIndex, endpt);
		USETW(req.wLength, 3);
		break;
	case UAUDIO_VERSION2:
		req.bmRequestType = set ?
			UT_WRITE_CLASS_INTERFACE
			: UT_READ_CLASS_INTERFACE;
		req.bRequest = V2_CUR;
		USETW2(req.wValue, SAMPLING_FREQ_CONTROL, 0);
		USETW2(req.wIndex, clkid, sc->sc_ac_iface);
		USETW(req.wLength, 4);
		break;
	}

	return usbd_do_request(sc->sc_udev, &req, data);
}

Static usbd_status
uaudio_set_speed(struct uaudio_softc *sc, int endpt, int clkid, u_int speed)
{
	uint8_t data[4];

	DPRINTFN(5, "endpt=%d clkid=%u speed=%u\n", endpt, clkid, speed);

	data[0] = speed;
	data[1] = speed >> 8;
	data[2] = speed >> 16;
	data[3] = speed >> 24;

	return uaudio_speed(sc, endpt, clkid, data, 1);
}

#ifdef UAUDIO_DEBUG
SYSCTL_SETUP(sysctl_hw_uaudio_setup, "sysctl hw.uaudio setup")
{
        int err;
        const struct sysctlnode *rnode;
        const struct sysctlnode *cnode;
 
        err = sysctl_createv(clog, 0, NULL, &rnode,
            CTLFLAG_PERMANENT, CTLTYPE_NODE, "uaudio",
            SYSCTL_DESCR("uaudio global controls"),
            NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
 
        if (err)
                goto fail;
 
        /* control debugging printfs */
        err = sysctl_createv(clog, 0, &rnode, &cnode,
            CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
            "debug", SYSCTL_DESCR("Enable debugging output"),
            NULL, 0, &uaudiodebug, sizeof(uaudiodebug), CTL_CREATE, CTL_EOL);
        if (err)
                goto fail;
 
        return;
fail:
        aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
} 
#endif

#ifdef _MODULE

MODULE(MODULE_CLASS_DRIVER, uaudio, NULL);

static const struct cfiattrdata audiobuscf_iattrdata = {
	"audiobus", 0, { { NULL, NULL, 0 }, }
};
static const struct cfiattrdata * const uaudio_attrs[] = {
	&audiobuscf_iattrdata, NULL
};
CFDRIVER_DECL(uaudio, DV_DULL, uaudio_attrs);
extern struct cfattach uaudio_ca;
static int uaudioloc[6/*USBIFIFCF_NLOCS*/] = {
	-1/*USBIFIFCF_PORT_DEFAULT*/,
	-1/*USBIFIFCF_CONFIGURATION_DEFAULT*/,
	-1/*USBIFIFCF_INTERFACE_DEFAULT*/,
	-1/*USBIFIFCF_VENDOR_DEFAULT*/,
	-1/*USBIFIFCF_PRODUCT_DEFAULT*/,
	-1/*USBIFIFCF_RELEASE_DEFAULT*/};
static struct cfparent uhubparent = {
	"usbifif", NULL, DVUNIT_ANY
};
static struct cfdata uaudio_cfdata[] = {
	{
		.cf_name = "uaudio",
		.cf_atname = "uaudio",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = uaudioloc,
		.cf_flags = 0,
		.cf_pspec = &uhubparent,
	},
	{ NULL }
};

static int
uaudio_modcmd(modcmd_t cmd, void *arg)
{
	int err;

	switch (cmd) {
	case MODULE_CMD_INIT:
		err = config_cfdriver_attach(&uaudio_cd);
		if (err) {
			return err;
		}
		err = config_cfattach_attach("uaudio", &uaudio_ca);
		if (err) {
			config_cfdriver_detach(&uaudio_cd);
			return err;
		}
		err = config_cfdata_attach(uaudio_cfdata, 1);
		if (err) {
			config_cfattach_detach("uaudio", &uaudio_ca);
			config_cfdriver_detach(&uaudio_cd);
			return err;
		}
		return 0;
	case MODULE_CMD_FINI:
		err = config_cfdata_detach(uaudio_cfdata);
		if (err)
			return err;
		config_cfattach_detach("uaudio", &uaudio_ca);
		config_cfdriver_detach(&uaudio_cd);
		return 0;
	default:
		return ENOTTY;
	}
}

#endif
