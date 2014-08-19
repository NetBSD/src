/*	$NetBSD: ffb.c,v 1.52.2.1 2014/08/20 00:03:25 tls Exp $	*/
/*	$OpenBSD: creator.c,v 1.20 2002/07/30 19:48:15 jason Exp $	*/

/*
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net)
 * All rights reserved.
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
 *	This product includes software developed by Jason L. Wright
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffb.c,v 1.52.2.1 2014/08/20 00:03:25 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/vmparam.h>

#include <dev/wscons/wsconsio.h>
#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>

#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <prop/proplib.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>
#include <dev/i2c/ddcvar.h>

#include <sparc64/dev/ffbreg.h>
#include <sparc64/dev/ffbvar.h>

#include "opt_wsdisplay_compat.h"
#include "opt_ffb.h"

#ifndef WS_DEFAULT_BG
/* Sun -> background should be white */
#define WS_DEFAULT_BG 0xf
#endif

#ifdef FFB_SYNC
#define SYNC ffb_ras_wait(sc)
#else
#define SYNC
#endif

/* Debugging */
#if !defined FFB_DEBUG
#define FFB_DEBUG 0
#endif
#define DPRINTF(x)	if (ffb_debug) printf x
/* Patchable */
extern int ffb_debug;
#if FFB_DEBUG > 0
int ffb_debug = 1;
#else
int ffb_debug = 0;
#endif

extern struct cfdriver ffb_cd;

struct wsscreen_descr ffb_stdscreen = {
	"sunffb",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global. */
	0,
	0, 0,
	WSSCREEN_REVERSE | WSSCREEN_WSCOLORS,
	NULL	/* modecookie */
};

const struct wsscreen_descr *ffb_scrlist[] = {
	&ffb_stdscreen,
	/* XXX other formats? */
};

struct wsscreen_list ffb_screenlist = {
	sizeof(ffb_scrlist) / sizeof(struct wsscreen_descr *),
	    ffb_scrlist
};

static struct vcons_screen ffb_console_screen;

int	ffb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static int ffb_blank(struct ffb_softc *, u_long, u_int *);
paddr_t ffb_mmap(void *, void *, off_t, int);
void	ffb_ras_fifo_wait(struct ffb_softc *, int);
void	ffb_ras_wait(struct ffb_softc *);
void	ffb_ras_init(struct ffb_softc *);
void	ffb_ras_copyrows(void *, int, int, int);
void	ffb_ras_erasecols(void *, int, int, int, long int);
void	ffb_ras_eraserows(void *, int, int, long int);
void	ffb_ras_do_cursor(struct rasops_info *);
void	ffb_ras_fill(struct ffb_softc *);
void	ffb_ras_invert(struct ffb_softc *);
static void	ffb_ras_setfg(struct ffb_softc *, int32_t);
static void	ffb_ras_setbg(struct ffb_softc *, int32_t);

void	ffb_clearscreen(struct ffb_softc *);
int	ffb_load_font(void *, void *, struct wsdisplay_font *);
void	ffb_init_screen(void *, struct vcons_screen *, int, 
	    long *);
int	ffb_allocattr(void *, int, int, int, long *);
void	ffb_putchar_mono(void *, int, int, u_int, long);
void	ffb_putchar_aa(void *, int, int, u_int, long);
void	ffb_cursor(void *, int, int, int);

/* frame buffer generic driver */   
static void ffbfb_unblank(device_t);
dev_type_open(ffbfb_open);
dev_type_close(ffbfb_close);
dev_type_ioctl(ffbfb_ioctl);
dev_type_mmap(ffbfb_mmap);

static struct fbdriver ffb_fbdriver = {
        ffbfb_unblank, ffbfb_open, ffbfb_close, ffbfb_ioctl, nopoll,
	ffbfb_mmap, nokqfilter
};

struct wsdisplay_accessops ffb_accessops = {
	.ioctl = ffb_ioctl,
	.mmap = ffb_mmap,
};

/* I2C glue */
static int ffb_i2c_acquire_bus(void *, int);
static void ffb_i2c_release_bus(void *, int);
static int ffb_i2c_send_start(void *, int);
static int ffb_i2c_send_stop(void *, int);
static int ffb_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int ffb_i2c_read_byte(void *, uint8_t *, int);
static int ffb_i2c_write_byte(void *, uint8_t, int);

/* I2C bitbang glue */
static void ffb_i2cbb_set_bits(void *, uint32_t);
static void ffb_i2cbb_set_dir(void *, uint32_t);
static uint32_t ffb_i2cbb_read(void *);

static const struct i2c_bitbang_ops ffb_i2cbb_ops = {
	ffb_i2cbb_set_bits,
	ffb_i2cbb_set_dir,
	ffb_i2cbb_read,
	{
		FFB_DAC_CFG_MPDATA_SDA,
		FFB_DAC_CFG_MPDATA_SCL,
		0,
		0
	}
};

void ffb_attach_i2c(struct ffb_softc *);

/* Video mode setting */
int ffb_tgc_disable(struct ffb_softc *);
void ffb_get_pclk(int, uint32_t *, int *);
int ffb_set_vmode(struct ffb_softc *, struct videomode *, int, int *, int *);


void
ffb_attach(device_t self)
{
	struct ffb_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args waa;
	struct rasops_info *ri;
	long defattr;
	const char *model, *out_dev;
	int btype;
	uint32_t dac;
	int maxrow;
	u_int blank = WSDISPLAYIO_VIDEO_ON;
	char buf[6+1];
	int i, try_edid;
	prop_data_t data;

	printf(":");
		
	if (sc->sc_type == FFB_CREATOR) {
		btype = prom_getpropint(sc->sc_node, "board_type", 0);
		if ((btype & 7) == 3)
			printf(" Creator3D");
		else
			printf(" Creator");
	} else {
		printf(" Elite3D");
		btype = 0;
	}

	model = prom_getpropstring(sc->sc_node, "model");
	if (model == NULL || strlen(model) == 0)
		model = "unknown";

	sc->sc_depth = 24;
	sc->sc_linebytes = 8192;
	/* We might alter these during EDID mode setting */
	sc->sc_height = prom_getpropint(sc->sc_node, "height", 0);
	sc->sc_width = prom_getpropint(sc->sc_node, "width", 0);
	
	sc->sc_locked = 0;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	
	maxrow = (prom_getoption("screen-#rows", buf, sizeof buf) != 0)
		? strtoul(buf, NULL, 10)
		: 34;

	/* collect DAC version, as Elite3D cursor enable bit is reversed */
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_DEVID);
	dac = DAC_READ(sc, FFB_DAC_VALUE);
	sc->sc_dacrev = (dac >> 28) & 0xf;

	if (sc->sc_type == FFB_AFB) {
		sc->sc_dacrev = 10;
		sc->sc_needredraw = 0;
	} else {
		/* see what kind of DAC we have */
		int pnum = (dac & 0x0ffff000) >> 12;
		if (pnum == 0x236e) {
			sc->sc_needredraw = 0;
		} else {
			sc->sc_needredraw = 1;
		}
	}
	printf(", model %s, dac %u\n", model, sc->sc_dacrev);
	if (sc->sc_needredraw) 
		printf("%s: found old DAC, enabling redraw on unblank\n", 
		    device_xname(sc->sc_dev));

	/* Check if a console resolution "<device>:r<res>" is set. */
	if (sc->sc_console) {
		out_dev = prom_getpropstring(sc->sc_node, "output_device");
		if (out_dev != NULL && strlen(out_dev) != 0 &&
		    strstr(out_dev, ":r") != NULL)
			try_edid = 0;
		else
			try_edid = 1;
	} else
		try_edid = 1;

#if FFB_DEBUG > 0
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_TGC);
	printf("tgc: %08x\n", DAC_READ(sc, FFB_DAC_VALUE));
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_DAC_CTRL);
	printf("dcl: %08x\n", DAC_READ(sc, FFB_DAC_VALUE));
#endif
	ffb_attach_i2c(sc);

	/* Need to set asynchronous blank during DDC write/read */
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_USR_CTRL);
	dac = DAC_READ(sc, FFB_DAC_VALUE);
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_USR_CTRL);
	DAC_WRITE(sc, FFB_DAC_VALUE, dac | FFB_DAC_USR_CTRL_BLANK);

	/* Some monitors don't respond first time */
	i = 0;
	while (sc->sc_edid_data[1] == 0 && i++ < 3)
		ddc_read_edid(&sc->sc_i2c, sc->sc_edid_data, EDID_DATA_LEN);

	/* Remove asynchronous blank */
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_USR_CTRL);
	DAC_WRITE(sc, FFB_DAC_VALUE, dac);

	if (edid_parse(&sc->sc_edid_data[0], &sc->sc_edid_info) != -1) {
		sort_modes(sc->sc_edid_info.edid_modes,
		    &sc->sc_edid_info.edid_preferred_mode,
		    sc->sc_edid_info.edid_nmodes);
		DPRINTF(("%s: EDID data:\n  ", device_xname(sc->sc_dev)));
		for (i = 0; i < EDID_DATA_LEN; i++) {
			if (i && !(i % 32))
				DPRINTF(("\n "));
			if (i && !(i % 4))
				DPRINTF((" "));
			DPRINTF(("%02x", sc->sc_edid_data[i]));
		}
		DPRINTF(("\n"));
		if (ffb_debug)
			edid_print(&sc->sc_edid_info);

		data = prop_data_create_data(sc->sc_edid_data, EDID_DATA_LEN);
		prop_dictionary_set(device_properties(self), "EDID", data);
		prop_object_release(data);

		if (try_edid)
			for (i = 0; i < sc->sc_edid_info.edid_nmodes; i++) {
				if (ffb_set_vmode(sc,
			    	    &(sc->sc_edid_info.edid_modes[i]), btype,
				    &(sc->sc_width), &(sc->sc_height)))
					break;
			}
	} else {
		DPRINTF(("%s: No EDID data.\n", device_xname(sc->sc_dev)));
	}
		
	ffb_ras_init(sc);

	ffb_blank(sc, WSDISPLAYIO_SVIDEO, &blank);

	sc->sc_accel = ((device_cfdata(sc->sc_dev)->cf_flags &
	    FFB_CFFLAG_NOACCEL) == 0);
		
	wsfont_init();

	vcons_init(&sc->vd, sc, &ffb_stdscreen, &ffb_accessops);
	sc->vd.init_screen = ffb_init_screen;
	ri = &ffb_console_screen.scr_ri;

	/* we mess with ffb_console_screen only once */
	if (sc->sc_console) {
		vcons_init_screen(&sc->vd, &ffb_console_screen, 1, &defattr);
		SCREEN_VISIBLE((&ffb_console_screen));
		/* 
		 * XXX we shouldn't use a global variable for the console
		 * screen
		 */
		sc->vd.active = &ffb_console_screen;
		ffb_console_screen.scr_flags = VCONS_SCREEN_IS_STATIC;
	} else {
		if (ffb_console_screen.scr_ri.ri_rows == 0) {
			/* do some minimal setup to avoid weirdnesses later */
			vcons_init_screen(&sc->vd, &ffb_console_screen, 1, &defattr);
		} else
			(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	}

	ffb_stdscreen.nrows = ri->ri_rows;
	ffb_stdscreen.ncols = ri->ri_cols;
	ffb_stdscreen.textops = &ri->ri_ops;
	ffb_stdscreen.capabilities = ri->ri_caps;
	
	sc->sc_fb.fb_driver = &ffb_fbdriver;
	sc->sc_fb.fb_type.fb_cmsize = 0;
	sc->sc_fb.fb_type.fb_size = maxrow * sc->sc_linebytes;
	sc->sc_fb.fb_type.fb_type = FBTYPE_CREATOR;
	sc->sc_fb.fb_type.fb_width = sc->sc_width;
	sc->sc_fb.fb_type.fb_depth = sc->sc_depth;
	sc->sc_fb.fb_type.fb_height = sc->sc_height;
	sc->sc_fb.fb_device = sc->sc_dev;
	fb_attach(&sc->sc_fb, sc->sc_console);

	ffb_clearscreen(sc);

	if (sc->sc_console) {
		wsdisplay_cnattach(&ffb_stdscreen, ri, 0, 0, defattr);
		vcons_replay_msgbuf(&ffb_console_screen);
	}
	
	waa.console = sc->sc_console;
	waa.scrdata = &ffb_screenlist;
	waa.accessops = &ffb_accessops;
	waa.accesscookie = &sc->vd;
	config_found(sc->sc_dev, &waa, wsemuldisplaydevprint);
}

void
ffb_attach_i2c(struct ffb_softc *sc)
{

	/* Fill in the i2c tag */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = ffb_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = ffb_i2c_release_bus;
	sc->sc_i2c.ic_send_start = ffb_i2c_send_start;
	sc->sc_i2c.ic_send_stop = ffb_i2c_send_stop;
	sc->sc_i2c.ic_initiate_xfer = ffb_i2c_initiate_xfer;
	sc->sc_i2c.ic_read_byte = ffb_i2c_read_byte;
	sc->sc_i2c.ic_write_byte = ffb_i2c_write_byte;
	sc->sc_i2c.ic_exec = NULL;
}

int
ffb_ioctl(void *v, void *vs, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct vcons_data *vd = v;
	struct ffb_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	DPRINTF(("ffb_ioctl: %s cmd _IO%s%s('%c', %lu)\n",
	       device_xname(sc->sc_dev),
	       (cmd & IOC_IN) ? "W" : "", (cmd & IOC_OUT) ? "R" : "",
	       (char)IOCGROUP(cmd), cmd & 0xff));

	switch (cmd) {
	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;
	case FBIOGATTR:
#define fba ((struct fbgattr *)data)
		fba->real_type = sc->sc_fb.fb_type.fb_type;
		fba->owner = 0; 	/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = -1;
#undef fba
		break; 

	case FBIOGETCMAP:
	case FBIOPUTCMAP:
		return EIO;

	case FBIOGVIDEO:
	case FBIOSVIDEO:
		return ffb_blank(sc, cmd == FBIOGVIDEO?
		    WSDISPLAYIO_GVIDEO : WSDISPLAYIO_SVIDEO,
		    (u_int *)data);
		break;
	case FBIOGCURSOR:
	case FBIOSCURSOR:
		/* the console driver is not using the hardware cursor */
		break;
	case FBIOGCURPOS:
		printf("%s: FBIOGCURPOS not implemented\n",
		    device_xname(sc->sc_dev));
		return EIO;
	case FBIOSCURPOS:
		printf("%s: FBIOSCURPOS not implemented\n",
		    device_xname(sc->sc_dev));
		return EIO;
	case FBIOGCURMAX:
		printf("%s: FBIOGCURMAX not implemented\n",
		    device_xname(sc->sc_dev));
		return EIO;

	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_SUNFFB;
		break;
	case WSDISPLAYIO_SMODE:
		{ 
			if (sc->sc_mode != *(u_int *)data) {
				sc->sc_mode = *(u_int *)data;
				if ((sc->sc_mode == WSDISPLAYIO_MODE_EMUL) &&
				    (sc->sc_locked == 0)) {
					ffb_ras_init(sc);		
					vcons_redraw_screen(ms);
				}
			}
		}		
		break;
	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = sc->sc_height;
		wdf->width  = sc->sc_width;
		wdf->depth  = 32;
		wdf->cmsize = 256; /* XXX */
		break;
#ifdef WSDISPLAYIO_LINEBYTES
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_linebytes;
		break;
#endif
	case WSDISPLAYIO_GETCMAP:
		break;/* XXX */

	case WSDISPLAYIO_PUTCMAP:
		break;/* XXX */

	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
		return(ffb_blank(sc, cmd, (u_int *)data));
		break;
	
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		return EIO; /* not supported yet */
		break;
	
	case WSDISPLAYIO_GET_EDID: {
		struct wsdisplayio_edid_info *d = data;
		return wsdisplayio_get_edid(sc->sc_dev, d);
	}
	
	case WSDISPLAYIO_GET_FBINFO: {
		struct wsdisplayio_fbinfo *fbi = data;
		return wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
	}
	
	default:
		return EPASSTHROUGH;
	}

	return (0);
}

/* blank/unblank the screen */
static int
ffb_blank(struct ffb_softc *sc, u_long cmd, u_int *data)
{
	struct vcons_screen *ms = sc->vd.active;
	u_int val;
	
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_TGC);
	val = DAC_READ(sc, FFB_DAC_VALUE);

	switch (cmd) {
	case WSDISPLAYIO_GVIDEO:
		*data = val & 1;
		return(0);
		break;
	case WSDISPLAYIO_SVIDEO:
		if (*data == WSDISPLAYIO_VIDEO_OFF)
			val &= ~1;
		else if (*data == WSDISPLAYIO_VIDEO_ON)
			val |= 1;
		else
			return(EINVAL);
		break;
	default:
		return(EINVAL);
	}

	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_TGC);
	DAC_WRITE(sc, FFB_DAC_VALUE, val);
	
	if ((val & 1) && sc->sc_needredraw) {
		if (ms != NULL) {
			if ((sc->sc_mode == WSDISPLAYIO_MODE_EMUL) && 
			    (sc->sc_locked == 0)) {
				ffb_ras_init(sc);		
				vcons_redraw_screen(ms);
			}
		}
	}

	return(0);
}

paddr_t
ffb_mmap(void *vsc, void *vs, off_t off, int prot)
{
	struct vcons_data *vd = vsc;
	struct ffb_softc *sc = vd->cookie;
	int i;

	switch (sc->sc_mode) {
	case WSDISPLAYIO_MODE_MAPPED:
		for (i = 0; i < sc->sc_nreg; i++) {
			/* Before this set? */
			if (off < sc->sc_addrs[i])
				continue;
			/* After this set? */
			if (off >= (sc->sc_addrs[i] + sc->sc_sizes[i]))
				continue;

			return (bus_space_mmap(sc->sc_bt, sc->sc_addrs[i],
			    off - sc->sc_addrs[i], prot, BUS_SPACE_MAP_LINEAR));
		}
		break;
#ifdef WSDISPLAYIO_MODE_DUMBFB
	case WSDISPLAYIO_MODE_DUMBFB:
		if (sc->sc_nreg < FFB_REG_DFB24)
			break;
		if (off >= 0 && off < sc->sc_sizes[FFB_REG_DFB24])
			return (bus_space_mmap(sc->sc_bt,
			    sc->sc_addrs[FFB_REG_DFB24], off, prot,
			    BUS_SPACE_MAP_LINEAR));
		break;
#endif
	}
	return (-1);
}

void
ffb_ras_fifo_wait(struct ffb_softc *sc, int n)
{
	int32_t cache = sc->sc_fifo_cache;

	if (cache < n) {
		do {
			cache = FBC_READ(sc, FFB_FBC_UCSR);
			cache = (cache & FBC_UCSR_FIFO_MASK) - 8;
		} while (cache < n);
	}
	sc->sc_fifo_cache = cache - n;
}

void
ffb_ras_wait(struct ffb_softc *sc)
{
	uint32_t ucsr, r;

	while (1) {
		ucsr = FBC_READ(sc, FFB_FBC_UCSR);
		if ((ucsr & (FBC_UCSR_FB_BUSY|FBC_UCSR_RP_BUSY)) == 0)
			break;
		r = ucsr & (FBC_UCSR_READ_ERR | FBC_UCSR_FIFO_OVFL);
		if (r != 0)
			FBC_WRITE(sc, FFB_FBC_UCSR, r);
	}
}

void
ffb_ras_init(struct ffb_softc *sc)
{
	uint32_t fbc;

	if (sc->sc_width > 1280) {
	DPRINTF(("ffb_ras_init: high resolution.\n"));
		fbc = FFB_FBC_WM_COMBINED | FFB_FBC_WE_FORCEON |
		    FFB_FBC_ZE_OFF | FFB_FBC_YE_OFF | FFB_FBC_XE_ON;
	} else {
	DPRINTF(("ffb_ras_init: standard resolution.\n"));
		fbc = FFB_FBC_XE_OFF;
	}
	ffb_ras_fifo_wait(sc, 11);
	DPRINTF(("WID: %08x\n", FBC_READ(sc, FFB_FBC_WID)));
	FBC_WRITE(sc, FFB_FBC_WID, 0x0);
	FBC_WRITE(sc, FFB_FBC_PPC,
	    FBC_PPC_VCE_DIS | FBC_PPC_TBE_OPAQUE | FBC_PPC_ACE_DIS | 
	    FBC_PPC_APE_DIS | FBC_PPC_DCE_DIS | FBC_PPC_CS_CONST | 
	    FBC_PPC_ABE_DIS | FBC_PPC_XS_WID);
	    
	fbc |= FFB_FBC_WB_A | FFB_FBC_RB_A | FFB_FBC_SB_BOTH |
	       FFB_FBC_RGBE_MASK;
        DPRINTF(("%s: fbc is %08x\n", __func__, fbc));
        FBC_WRITE(sc, FFB_FBC_FBC, fbc);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_NEW);
	FBC_WRITE(sc, FFB_FBC_DRAWOP, FBC_DRAWOP_RECTANGLE);
	FBC_WRITE(sc, FFB_FBC_PMASK, 0xffffffff);
	FBC_WRITE(sc, FFB_FBC_FONTINC, 0x10000);
	sc->sc_fg_cache = 0;
	FBC_WRITE(sc, FFB_FBC_FG, sc->sc_fg_cache);
	FBC_WRITE(sc, FFB_FBC_BLENDC, FFB_BLENDC_FORCE_ONE |
				      FFB_BLENDC_DF_ONE_M_A |
				      FFB_BLENDC_SF_A);
	FBC_WRITE(sc, FFB_FBC_BLENDC1, 0);
	FBC_WRITE(sc, FFB_FBC_BLENDC2, 0);
	ffb_ras_wait(sc);
}

void
ffb_ras_eraserows(void *cookie, int row, int n, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->scr_cookie;

	if (row < 0) {
		n += row;
		row = 0;
	}
	if (row + n > ri->ri_rows)
		n = ri->ri_rows - row;
	if (n <= 0)
		return;
	
	ffb_ras_fill(sc);
	ffb_ras_setfg(sc, ri->ri_devcmap[(attr >> 16) & 0xf]);
	ffb_ras_fifo_wait(sc, 4);
	if ((n == ri->ri_rows) && (ri->ri_flg & RI_FULLCLEAR)) {
		FBC_WRITE(sc, FFB_FBC_BY, 0);
		FBC_WRITE(sc, FFB_FBC_BX, 0);
		FBC_WRITE(sc, FFB_FBC_BH, ri->ri_height);
		FBC_WRITE(sc, FFB_FBC_BW, ri->ri_width);
	} else {
		row *= ri->ri_font->fontheight;
		FBC_WRITE(sc, FFB_FBC_BY, ri->ri_yorigin + row);
		FBC_WRITE(sc, FFB_FBC_BX, ri->ri_xorigin);
		FBC_WRITE(sc, FFB_FBC_BH, n * ri->ri_font->fontheight);
		FBC_WRITE(sc, FFB_FBC_BW, ri->ri_emuwidth);
	}
	SYNC;
}

void
ffb_ras_erasecols(void *cookie, int row, int col, int n, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->scr_cookie;

	if ((row < 0) || (row >= ri->ri_rows))
		return;
	if (col < 0) {
		n += col;
		col = 0;
	}
	if (col + n > ri->ri_cols)
		n = ri->ri_cols - col;
	if (n <= 0)
		return;
	n *= ri->ri_font->fontwidth;
	col *= ri->ri_font->fontwidth;
	row *= ri->ri_font->fontheight;

	ffb_ras_fill(sc);
	ffb_ras_setfg(sc, ri->ri_devcmap[(attr >> 16) & 0xf]);
	ffb_ras_fifo_wait(sc, 4);
	FBC_WRITE(sc, FFB_FBC_BY, ri->ri_yorigin + row);
	FBC_WRITE(sc, FFB_FBC_BX, ri->ri_xorigin + col);
	FBC_WRITE(sc, FFB_FBC_BH, ri->ri_font->fontheight);
	FBC_WRITE(sc, FFB_FBC_BW, n - 1);
	SYNC;
}

void
ffb_ras_fill(struct ffb_softc *sc)
{
	ffb_ras_fifo_wait(sc, 3);
	FBC_WRITE(sc, FFB_FBC_PPC,
	    FBC_PPC_VCE_DIS | FBC_PPC_TBE_OPAQUE | FBC_PPC_ACE_DIS | 
	    FBC_PPC_APE_DIS | FBC_PPC_DCE_DIS | FBC_PPC_CS_CONST | 
	    FBC_PPC_ABE_DIS | FBC_PPC_XS_WID);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_NEW);
	FBC_WRITE(sc, FFB_FBC_DRAWOP, FBC_DRAWOP_RECTANGLE);
	SYNC;
}

void
ffb_ras_invert(struct ffb_softc *sc)
{
	ffb_ras_fifo_wait(sc, 3);
	FBC_WRITE(sc, FFB_FBC_PPC,
	    FBC_PPC_VCE_DIS | FBC_PPC_TBE_OPAQUE | FBC_PPC_ACE_DIS | 
	    FBC_PPC_APE_DIS | FBC_PPC_DCE_DIS | FBC_PPC_CS_CONST | 
	    FBC_PPC_ABE_DIS | FBC_PPC_XS_WID);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_INVERT);
	FBC_WRITE(sc, FFB_FBC_DRAWOP, FBC_DRAWOP_RECTANGLE);
	SYNC;
}

void
ffb_ras_copyrows(void *cookie, int src, int dst, int n)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct ffb_softc *sc = scr->scr_cookie;

	if (dst == src)
		return;
	if (src < 0) {
		n += src;
		src = 0;
	}
	if ((src + n) > ri->ri_rows)
		n = ri->ri_rows - src;
	if (dst < 0) {
		n += dst;
		dst = 0;
	}
	if ((dst + n) > ri->ri_rows)
		n = ri->ri_rows - dst;
	if (n <= 0)
		return;
	n *= ri->ri_font->fontheight;
	src *= ri->ri_font->fontheight;
	dst *= ri->ri_font->fontheight;

	ffb_ras_fifo_wait(sc, 9);
	FBC_WRITE(sc, FFB_FBC_PPC,
	    FBC_PPC_VCE_DIS | FBC_PPC_TBE_OPAQUE | FBC_PPC_ACE_DIS | 
	    FBC_PPC_APE_DIS | FBC_PPC_DCE_DIS | FBC_PPC_CS_CONST | 
	    FBC_PPC_ABE_DIS | FBC_PPC_XS_WID);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_OLD | (FBC_ROP_OLD << 8));
	FBC_WRITE(sc, FFB_FBC_DRAWOP, FBC_DRAWOP_VSCROLL);
	FBC_WRITE(sc, FFB_FBC_BY, ri->ri_yorigin + src);
	FBC_WRITE(sc, FFB_FBC_BX, ri->ri_xorigin);
	FBC_WRITE(sc, FFB_FBC_DY, ri->ri_yorigin + dst);
	FBC_WRITE(sc, FFB_FBC_DX, ri->ri_xorigin);
	FBC_WRITE(sc, FFB_FBC_BH, n);
	FBC_WRITE(sc, FFB_FBC_BW, ri->ri_emuwidth);
	SYNC;
}

static void
ffb_ras_setfg(struct ffb_softc *sc, int32_t fg)
{
	ffb_ras_fifo_wait(sc, 1);
	if (fg == sc->sc_fg_cache)
		return;
	sc->sc_fg_cache = fg;
	FBC_WRITE(sc, FFB_FBC_FG, fg);
	SYNC;
}

static void
ffb_ras_setbg(struct ffb_softc *sc, int32_t bg)
{
	ffb_ras_fifo_wait(sc, 1);
	if (bg == sc->sc_bg_cache)
		return;
	sc->sc_bg_cache = bg;
	FBC_WRITE(sc, FFB_FBC_BG, bg);
	SYNC;
}

/* frame buffer generic driver support functions */   
static void
ffbfb_unblank(device_t dev)
{
	struct ffb_softc *sc = device_private(dev);
	struct vcons_screen *ms = sc->vd.active;
	u_int on = 1;
	int redraw = 0;
	
	ffb_ras_init(sc);
	if (sc->sc_locked) {
		sc->sc_locked = 0;
		redraw = 1;
	}
	
	ffb_blank(sc, WSDISPLAYIO_SVIDEO, &on);
#if 0
	if ((sc->vd.active != &ffb_console_screen) &&
	    (ffb_console_screen.scr_flags & VCONS_SCREEN_IS_STATIC)) {
		/* 
		 * force-switch to the console screen.
		 * Caveat: the higher layer will think we're still on the
		 * other screen
		 */
		
		SCREEN_INVISIBLE(sc->vd.active);
		sc->vd.active = &ffb_console_screen;
		SCREEN_VISIBLE(sc->vd.active);
		ms = sc->vd.active;
		redraw = 1;
	}
#endif	
	if (redraw) {
		vcons_redraw_screen(ms);
	}
}

int
ffbfb_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct ffb_softc *sc;

	sc = device_lookup_private(&ffb_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
		
	sc->sc_locked = 1;
	return 0;
}

int
ffbfb_close(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct ffb_softc *sc = device_lookup_private(&ffb_cd, minor(dev));
	struct vcons_screen *ms = sc->vd.active;
	
	sc->sc_locked = 0;
	if (ms != NULL) {
		if ((sc->sc_mode == WSDISPLAYIO_MODE_EMUL) &&
		    (sc->sc_locked == 0)) {
			ffb_ras_init(sc);		
			vcons_redraw_screen(ms);
		}
	}
	return 0;
}

int
ffbfb_ioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct ffb_softc *sc = device_lookup_private(&ffb_cd, minor(dev));

	return ffb_ioctl(&sc->vd, NULL, cmd, data, flags, l);
}

paddr_t
ffbfb_mmap(dev_t dev, off_t off, int prot)
{
	struct ffb_softc *sc = device_lookup_private(&ffb_cd, minor(dev));
	uint64_t size;
	int i, reg;
	off_t o;

	/*
	 * off is a magic cookie (see xfree86/drivers/sunffb/ffb.h),
	 * which we map to an index into the "reg" property, and use
	 * our copy of the firmware data as arguments for the real
	 * mapping.
	 */
	static struct { unsigned long voff; int reg; } map[] = {
		{ 0x00000000, FFB_REG_SFB8R },
		{ 0x00400000, FFB_REG_SFB8G },
		{ 0x00800000, FFB_REG_SFB8B },
		{ 0x00c00000, FFB_REG_SFB8X },
		{ 0x01000000, FFB_REG_SFB32 },
		{ 0x02000000, FFB_REG_SFB64  },
		{ 0x04000000, FFB_REG_FBC },
		{ 0x04004000, FFB_REG_DFB8R },
		{ 0x04404000, FFB_REG_DFB8G },
		{ 0x04804000, FFB_REG_DFB8B },
		{ 0x04c04000, FFB_REG_DFB8X },
		{ 0x05004000, FFB_REG_DFB24 },
		{ 0x06004000, FFB_REG_DFB32 },
		{ 0x07004000, FFB_REG_DFB422A },
		{ 0x0bc06000, FFB_REG_DAC },
		{ 0x0bc08000, FFB_REG_PROM },
		{ 0x0bc18000, 0 }
	};

	/* special value "FFB_EXP_VOFF" - not backed by any "reg" entry */
	if (off == 0x0bc18000)
		return bus_space_mmap(sc->sc_bt, sc->sc_addrs[FFB_REG_PROM],
		    0x00200000, prot, BUS_SPACE_MAP_LINEAR);
		    
	/* 
	 * FFB_VOFF_FBC_KREGS - used by afbinit to upload firmware. We should 
	 * probably mmap them only on afb boards 
	 */
	if ((off >= 0x0bc04000) && (off < 0x0bc06000))
		return bus_space_mmap(sc->sc_bt, sc->sc_addrs[FFB_REG_PROM], 
		    0x00610000 + (off - 0x0bc04000), prot, 
		    BUS_SPACE_MAP_LINEAR);
		    
#define NELEMS(arr) (sizeof(arr)/sizeof((arr)[0]))

	/* the map is ordered by voff */
	for (i = 0; i < NELEMS(map)-1; i++) {
		reg = map[i].reg;
		/* the number of entries in reg seems to vary */
		if (reg < sc->sc_nreg) {
			size = min((map[i + 1].voff - map[i].voff), 
			    sc->sc_sizes[reg]);
			if ((off >= map[i].voff) && 
			    (off < (map[i].voff + size))) {
				o = off - map[i].voff;
				return bus_space_mmap(sc->sc_bt, 
				    sc->sc_addrs[reg], o, prot, 
				    BUS_SPACE_MAP_LINEAR);
			}
		}
	}

	return -1;
}

void
ffb_clearscreen(struct ffb_softc *sc)
{
	struct rasops_info *ri = &ffb_console_screen.scr_ri;
	ffb_ras_fill(sc);
	ffb_ras_setfg(sc, ri->ri_devcmap[WS_DEFAULT_BG]);
	ffb_ras_fifo_wait(sc, 4);
	FBC_WRITE(sc, FFB_FBC_BY, 0);
	FBC_WRITE(sc, FFB_FBC_BX, 0);
	FBC_WRITE(sc, FFB_FBC_BH, sc->sc_height);
	FBC_WRITE(sc, FFB_FBC_BW, sc->sc_width);
}

void
ffb_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr;
	struct ffb_softc *sc;
	int x, y, wi, he;
	
	if (cookie != NULL) {
		scr = ri->ri_hw;
		sc = scr->scr_cookie;
	
		wi = ri->ri_font->fontwidth;
		he = ri->ri_font->fontheight;

		if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
	
			if (ri->ri_flg & RI_CURSOR) {

				/* remove cursor */
				x = ri->ri_ccol * wi + ri->ri_xorigin;
				y = ri->ri_crow * he + ri->ri_yorigin;

				ffb_ras_invert(sc);
				ffb_ras_fifo_wait(sc, 4);
				FBC_WRITE(sc, FFB_FBC_BY, y);
				FBC_WRITE(sc, FFB_FBC_BX, x);
				FBC_WRITE(sc, FFB_FBC_BH, he);
				FBC_WRITE(sc, FFB_FBC_BW, wi);

				ri->ri_flg &= ~RI_CURSOR;
			}
			ri->ri_crow = row;
			ri->ri_ccol = col;
			if (on)
			{
				x = ri->ri_ccol * wi + ri->ri_xorigin;
				y = ri->ri_crow * he + ri->ri_yorigin;

				ffb_ras_invert(sc);
				ffb_ras_fifo_wait(sc, 4);
				FBC_WRITE(sc, FFB_FBC_BY, y);
				FBC_WRITE(sc, FFB_FBC_BX, x);
				FBC_WRITE(sc, FFB_FBC_BH, he);
				FBC_WRITE(sc, FFB_FBC_BW, wi);

				ri->ri_flg |= RI_CURSOR;
			}
		} else {
			ri->ri_crow = row;
			ri->ri_ccol = col;
			ri->ri_flg &= ~RI_CURSOR;
		}
	}
}

/* mono bitmap font */
void
ffb_putchar_mono(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct ffb_softc *sc = scr->scr_cookie;
	void *data;
	uint32_t fg, bg;
	int i;
	int x, y, wi, he;
	
	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	wi = font->fontwidth;
	he = font->fontheight;

	if (!CHAR_IN_FONT(c, font))
		return;

	bg = ri->ri_devcmap[(attr >> 16) & 0xf];
	fg = ri->ri_devcmap[(attr >> 24) & 0xf];
	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	data = WSFONT_GLYPH(c, font);

	ffb_ras_setbg(sc, bg);
	ffb_ras_setfg(sc, fg);
	ffb_ras_fifo_wait(sc, 4);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_NEW);
	FBC_WRITE(sc, FFB_FBC_FONTXY, (y << 16) | x);
	FBC_WRITE(sc, FFB_FBC_FONTW, wi);
	FBC_WRITE(sc, FFB_FBC_PPC,
	    FBC_PPC_VCE_DIS | FBC_PPC_TBE_OPAQUE | FBC_PPC_ACE_DIS | 
	    FBC_PPC_APE_DIS | FBC_PPC_DCE_DIS | FBC_PPC_CS_CONST | 
	    FBC_PPC_ABE_DIS | FBC_PPC_XS_WID);

	switch (font->stride) {
		case 1: {
			uint8_t *data8 = data;
			uint32_t reg;
			for (i = 0; i < he; i++) {
				reg = *data8;
				FBC_WRITE(sc, FFB_FBC_FONT, reg << 24);
				data8++;
			}
			break;
		}
		case 2: {
			uint16_t *data16 = data;
			uint32_t reg;
			for (i = 0; i < he; i++) {
				reg = *data16;
				FBC_WRITE(sc, FFB_FBC_FONT, reg << 16);
				data16++;
			}
			break;
		}
	}
}

/* alpha font */
void
ffb_putchar_aa(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct ffb_softc *sc = scr->scr_cookie;
	volatile uint32_t *dest, *ddest;
	uint8_t *data8;
	uint32_t fg, bg;
	int i;
	int x, y, wi, he;
	uint32_t alpha = 0x80;
	int j;
	
	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	wi = font->fontwidth;
	he = font->fontheight;

	if (!CHAR_IN_FONT(c, font))
		return;

	bg = ri->ri_devcmap[(attr >> 16) & 0xf];
	fg = ri->ri_devcmap[(attr >> 24) & 0xf];
	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	data8 = WSFONT_GLYPH(c, font);

	/* first we erase the background */
	ffb_ras_fill(sc);
	ffb_ras_setfg(sc, bg);
	ffb_ras_fifo_wait(sc, 4);
	FBC_WRITE(sc, FFB_FBC_BY, y);
	FBC_WRITE(sc, FFB_FBC_BX, x);
	FBC_WRITE(sc, FFB_FBC_BH, he);
	FBC_WRITE(sc, FFB_FBC_BW, wi);

	/* if we draw a space we're done */
	if (c == ' ') return;

	/* now enable alpha blending */
	ffb_ras_setfg(sc, fg);
	ffb_ras_fifo_wait(sc, 2);
	FBC_WRITE(sc, FFB_FBC_ROP, FBC_ROP_NEW);

	FBC_WRITE(sc, FFB_FBC_PPC,
	    FBC_PPC_VCE_DIS | FBC_PPC_TBE_OPAQUE | FBC_PPC_ACE_DIS | 
	    FBC_PPC_APE_DIS | FBC_PPC_DCE_DIS | FBC_PPC_CS_CONST | 
	    FBC_PPC_ABE_ENA | FBC_PPC_XS_VAR);
	/*
	 * we have to wait for both the rectangle drawing op above and the 
	 * FFB_FBC_PPC write to finish before mucking around in the SFB aperture
	 */
	ffb_ras_wait(sc);

	/* ... and draw the character */
	dest = sc->sc_sfb32 + (y << 11) + x;
	for (i = 0; i < he; i++) {
		ddest = dest;
		for (j = 0; j < wi; j++) {
			alpha = *data8;
			/*
			 * We set the colour source to constant above so we only
			 * have to write the alpha channel here and the colour
			 * comes from the FG register. It would be nice if we
			 * could just use the SFB8X aperture and memcpy() the
			 * alpha map line by line but for some strange reason
			 * that will take colour info from the framebuffer even
			 * if we set the FBC_PPC_CS_CONST bit above.
			 */
			*ddest = alpha << 24;
			data8++;
			ddest++;
		}
		dest += 2048;
	} 
}

int
ffb_allocattr(void *cookie, int fg, int bg, int flags, long *attrp)
{
	if ((fg == 0) && (bg == 0))
	{
		fg = WS_DEFAULT_FG;
		bg = WS_DEFAULT_BG;
	}
	if (flags & WSATTR_REVERSE) {
		*attrp = (bg & 0xff) << 24 | (fg & 0xff) << 16 | 
		    (flags & 0xff);
	} else
		*attrp = (fg & 0xff) << 24 | (bg & 0xff) << 16 | 
		    (flags & 0xff);
	return 0;
}

void
ffb_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct ffb_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 32;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_linebytes;
	ri->ri_flg = RI_CENTER | RI_ENABLE_ALPHA;

	/*
	 * we can't accelerate copycols() so instead of falling back to
	 * software use vcons' putchar() based implementation
	 */
	scr->scr_flags |= VCONS_NO_COPYCOLS;
#ifdef VCONS_DRAW_INTR
        scr->scr_flags |= VCONS_DONT_READ;
#endif
	DPRINTF(("ffb_init_screen: addr: %08lx\n",(ulong)ri->ri_bits));

	/* explicitly request BGR in case the default changes */
	ri->ri_rnum = 8;
	ri->ri_gnum = 8;
	ri->ri_bnum = 8;
	ri->ri_rpos = 0;
	ri->ri_gpos = 8;
	ri->ri_bpos = 16;

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	/* enable acceleration */
	ri->ri_ops.copyrows = ffb_ras_copyrows;
	ri->ri_ops.eraserows = ffb_ras_eraserows;
	ri->ri_ops.erasecols = ffb_ras_erasecols;
	ri->ri_ops.cursor = ffb_cursor;
	ri->ri_ops.allocattr = ffb_allocattr;
	if (FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar = ffb_putchar_aa;
	} else
		ri->ri_ops.putchar = ffb_putchar_mono;
}

/* I2C bitbanging */
static void ffb_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct ffb_softc *sc = cookie;

	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_CFG_MPDATA);
	DAC_WRITE(sc, FFB_DAC_VALUE, bits);
}

static void ffb_i2cbb_set_dir(void *cookie, uint32_t dir)
{
	/* Nothing to do */
}

static uint32_t ffb_i2cbb_read(void *cookie)
{
	struct ffb_softc *sc = cookie;
	uint32_t bits;

	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_CFG_MPSENSE);
	bits = DAC_READ(sc, FFB_DAC_VALUE);

	return bits;
}

/* higher level I2C stuff */
static int
ffb_i2c_acquire_bus(void *cookie, int flags)
{
	/* private bus */
	return (0);
}

static void
ffb_i2c_release_bus(void *cookie, int flags)
{
	/* private bus */
}

static int
ffb_i2c_send_start(void *cookie, int flags)
{
	return (i2c_bitbang_send_start(cookie, flags, &ffb_i2cbb_ops));
}

static int
ffb_i2c_send_stop(void *cookie, int flags)
{

	return (i2c_bitbang_send_stop(cookie, flags, &ffb_i2cbb_ops));
}

static int
ffb_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	/*
	 * for some reason i2c_bitbang_initiate_xfer left-shifts
	 * the I2C-address and then sets the direction bit
	 */
	return (i2c_bitbang_initiate_xfer(cookie, addr, flags, 
	    &ffb_i2cbb_ops));
}

static int
ffb_i2c_read_byte(void *cookie, uint8_t *valp, int flags)
{
	return (i2c_bitbang_read_byte(cookie, valp, flags, &ffb_i2cbb_ops));
}

static int
ffb_i2c_write_byte(void *cookie, uint8_t val, int flags)
{
	return (i2c_bitbang_write_byte(cookie, val, flags, &ffb_i2cbb_ops));
}


#define TVC_READ_LIMIT	100000
int
ffb_tgc_disable(struct ffb_softc *sc)
{
	int i;

	/* Is the timing generator disabled? */
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_TGC);
	if (!(DAC_READ(sc, FFB_DAC_VALUE) & FFB_DAC_TGC_TIMING_ENABLE))
		return 1;

	/* If not, disable it when the vertical counter reaches 0 */
	for (i = 0; i < TVC_READ_LIMIT; i++) {
		DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_TVC);
		if (!DAC_READ(sc, FFB_DAC_VALUE)) {
			DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_TGC);
			DAC_WRITE(sc, FFB_DAC_VALUE, 0);
			return 1;
		}
	}
	return 0;
}

/*
 * PLL Control Register values:
 *	M)ultiplier = bits 0:6 + 1
 *	D)ivisor = bits 7:10 + 1
 *	P)ost divisor = bits 11:13 (000 = 1, 001 = 2, 010 = 4, 011 = 8)
 *	Frequency = 13.5 * M / D / P
 */
#define FFB_PLL_FREQ	13500000
void
ffb_get_pclk(int request, uint32_t *pll, int *diff)
{
	int m, d, p, f, hex = 0, curdiff;

	*diff = 100000000;

	for (m = 32; m <= 80; m++) {
		for (d = 4; d <= 11; d++) {
			for (p = 1; p <= 8; p = p << 1) {
				switch (p) {
				case 1:
					hex = 0x4000 + (d << 7) + m;
					break;
				case 2:
					hex = 0x4800 + (d << 7) + m;
					break;
				case 4:
					hex = 0x5000 + (d << 7) + m;
					break;
				case 8:
					hex = 0x6000 + (d << 7) + m;
					break;
				}
				f = 13500000 * m / d / p;
				if (f == request) {
					*diff = 0;
					*pll = hex;
					return;
				} else {
					curdiff = abs(request - f);
					if (curdiff < *diff) {
						*diff = curdiff;
						*pll = hex;
					}
				}
			}
		}
	}
}

/*
 * Details of the FFB RAMDAC are contained in the Brooktree BT497/498
 * and in the Connexant BT497A/498A documentation.
 *
 * VESA timings to FFB register conversion:
 *	If interleave = 4/2:1 then x = 2, if interleave = 8/2:1 then x = 4
 *	VBE = VBS - vres = (sync pulse - 1) + back porch
 *	VBS = VSS - front porch = (sync pulse - 1) + back porch + vres
 *	VSE = sync pulse - 1
 *	VSS = (sync pulse - 1) + back porch + vres + front porch
 *	HRE = HSS - HSE - 1
 *	HBE = (sync pulse + back porch) / x - 1
 *	HBS = (sync pulse + back porch + hres) / x - 1
 *	HSE = sync pulse / x - 1
 *	HSS = (sync pulse + back porch + hres + front porch) / x - 1
 *	HCE = HBS - 4
 *	HCS = HBE - 4
 *	EPE = EIE = EIS = 0 (for all non-interlaced modes)
 *
 * Note, that 8/2:1 Single Buffered Interleaving is only supported by the
 * double-buffered FFB (Creator3D), and at higher resolutions than 1280x1024
 *
 * Note, that the timing generator should be disabled and re-enabled when the
 * the timing parameter registers are being programmed.  Stopping the timing
 * generator should only be done when the vertical counter is zero.
 */
#define DIVIDE(x,y)	(((x) + ((y) / 2)) / (y))
int
ffb_set_vmode(struct ffb_softc *sc, struct videomode *mode, int btype,
    int *hres, int *vres)
{
	int diff;
	uint32_t fp, sp, bp, x;
	uint32_t pll, pfc, ucl, dcl, tgc;
	uint32_t vbe, vbs, vse, vss, hre, hbe, hbs, hse, hss, hce, hcs;
	uint32_t epe, eie, eis;
	uint32_t fbcfg0;

	DPRINTF(("ffb_set_vmode: %dx%d@%d", mode->hdisplay, mode->vdisplay,
	    DIVIDE(DIVIDE(mode->dot_clock * 1000,
	    mode->htotal), mode->vtotal)));
	DPRINTF((" (%d %d %d %d %d %d %d",
	    mode->dot_clock, mode->hsync_start, mode->hsync_end, mode->htotal,
	    mode->vsync_start, mode->vsync_end, mode->vtotal));
	DPRINTF((" %s%sH %s%sV)\n",
	    mode->flags & VID_PHSYNC ? "+" : "",
	    mode->flags & VID_NHSYNC ? "-" : "",
	    mode->flags & VID_PVSYNC ? "+" : "",
	    mode->flags & VID_NVSYNC ? "-" : ""));

	/* We don't handle interlaced or doublescan (yet) */
	if ((mode->flags & VID_INTERLACE) || (mode->flags & VID_DBLSCAN))
		return 0;

	/* Only Creator3D can be set to > 1280x1024 */
	if(((sc->sc_type == FFB_CREATOR && !((btype & 7) == 3)) ||
	    sc->sc_type == FFB_AFB)
	    && (mode->hdisplay > 1280 || mode->vdisplay > 1024))
		return 0;
	/* Creator3D can be set to <= 1920x1360 */
	if (mode->hdisplay > 1920 || mode->vdisplay > 1360)
		return 0;

	/*
	 * Look for a matching pixel clock and set PLL Control.
	 * XXX: 640x480@60 is 25175000 in modelines but 25125000 in the
	 * FFB PROM, and the closest match to 25175000 (0x4da9/25159090)
	 * does not work.  So, use the PROM value instead.
	 */
	if (mode->hdisplay == 640 && mode->vdisplay == 480 &&
	    mode->dot_clock == 25175) {
		DPRINTF(("ffb_set_vmode: 640x480@60: adjusted dot clock\n"));
		mode->dot_clock = 25125;
	}
	ffb_get_pclk(mode->dot_clock * 1000, &pll, &diff);
	if (diff > 250000)
		return 0;
	
	/* Pixel Format Control, User Control and FBC Configuration. */
	if (mode->hdisplay > 1280) {
		pfc = FFB_DAC_PIX_FMT_821;
		ucl = FFB_DAC_USR_CTRL_OVERLAY | FFB_DAC_USR_CTRL_WMODE_C;
		x = 4;
		fbcfg0 = FBC_READ(sc, FFB_FBC_FBCFG0) | FBC_CFG0_DOUBLE_BUF;
	} else {
		pfc = FFB_DAC_PIX_FMT_421;
		/* Only Creator3D and Elite3D can have double-buffer */
		if ((sc->sc_type == FFB_CREATOR && !((btype & 7) == 3)))
			ucl = 0;
		else
			ucl = FFB_DAC_USR_CTRL_DOUBLE;
		ucl |= (FFB_DAC_USR_CTRL_OVERLAY | FFB_DAC_USR_CTRL_WMODE_S8);
		x = 2;
		fbcfg0 = FBC_READ(sc, FFB_FBC_FBCFG0) | FBC_CFG0_SINGLE_BUF;
	}

	/* DAC Control and Timing Generator Control */
	if (mode->flags & VID_PVSYNC)
		dcl = FFB_DAC_DAC_CTRL_POS_VSYNC;
	else
		dcl = 0;
	tgc = 0;
#define EDID_VID_INP	sc->sc_edid_info.edid_video_input
	if ((EDID_VID_INP & EDID_VIDEO_INPUT_COMPOSITE_SYNC)) {
		dcl |= FFB_DAC_DAC_CTRL_VSYNC_DIS;
		tgc = FFB_DAC_TGC_EQUAL_DISABLE;
	} else {
		dcl |= FFB_DAC_DAC_CTRL_SYNC_G;
		if (EDID_VID_INP & EDID_VIDEO_INPUT_SEPARATE_SYNCS)
			tgc |= FFB_DAC_TGC_VSYNC_DISABLE;
		else
			tgc = FFB_DAC_TGC_EQUAL_DISABLE;
	}
	if (EDID_VID_INP & EDID_VIDEO_INPUT_BLANK_TO_BLACK)
		dcl |= FFB_DAC_DAC_CTRL_PED_ENABLE;
	tgc |= (FFB_DAC_TGC_VIDEO_ENABLE | FFB_DAC_TGC_TIMING_ENABLE |
	    FFB_DAC_TGC_MASTER_ENABLE);

	/* Vertical timing */
	fp = mode->vsync_start - mode->vdisplay;
	sp = mode->vsync_end - mode->vsync_start;
	bp = mode->vtotal - mode->vsync_end;
	
	vbe = sp - 1 + bp;
	vbs = sp - 1 + bp + mode->vdisplay;
	vse = sp - 1;
	vss = sp  - 1 + bp + mode->vdisplay + fp;
	
	/* Horizontal timing */
	fp = mode->hsync_start - mode->hdisplay;
	sp = mode->hsync_end - mode->hsync_start;
	bp = mode->htotal - mode->hsync_end;

	hbe = (sp + bp) / x - 1;
	hbs = (sp + bp + mode->hdisplay) / x - 1;
	hse = sp / x - 1;
	hss = (sp + bp + mode->hdisplay + fp) / x -1;
	hre = hss - hse - 1;
	hce = hbs - 4;
	hcs = hbe - 4;

	/* Equalisation (interlaced modes) */
	epe = 0;
	eie = 0;
	eis = 0;

	DPRINTF(("ffb_set_vmode: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
	    pll, pfc, ucl, dcl, tgc));
	DPRINTF(("\t0x%04x 0x%04x 0x%04x 0x%04x\n", vbe, vbs, vse, vss));
	DPRINTF(("\t0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
	    hre, hbe, hbs, hse, hss, hce, hcs));
	DPRINTF(("\t0x%04x 0x%04x 0x%04x\n", epe, eie, eis));

	if (!ffb_tgc_disable(sc)) {
		DPRINTF(("ffb_set_vmode: failed to disable TGC register\n"));
		return 0;
	}

	/*
	 * Program the mode registers.
	 * Program the timing generator last, as that re-enables output.
	 * Note, that a read to/write from a register increments the
	 * register address to the next register automatically.
	 */
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_PLL_CTRL);
	DAC_WRITE(sc, FFB_DAC_VALUE, pll);
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_PIX_FMT);
	DAC_WRITE(sc, FFB_DAC_VALUE, pfc);
	DAC_WRITE(sc, FFB_DAC_VALUE, ucl);
	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_DAC_CTRL);
	DAC_WRITE(sc, FFB_DAC_VALUE, dcl);

	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_VBE);
	DAC_WRITE(sc, FFB_DAC_VALUE, vbe);
	DAC_WRITE(sc, FFB_DAC_VALUE, vbs);
	DAC_WRITE(sc, FFB_DAC_VALUE, vse);
	DAC_WRITE(sc, FFB_DAC_VALUE, vss);

	DAC_WRITE(sc, FFB_DAC_VALUE, hre);
	DAC_WRITE(sc, FFB_DAC_VALUE, hbe);
	DAC_WRITE(sc, FFB_DAC_VALUE, hbs);
	DAC_WRITE(sc, FFB_DAC_VALUE, hse);
	DAC_WRITE(sc, FFB_DAC_VALUE, hss);
	DAC_WRITE(sc, FFB_DAC_VALUE, hce);
	DAC_WRITE(sc, FFB_DAC_VALUE, hcs);

	DAC_WRITE(sc, FFB_DAC_VALUE, epe);
	DAC_WRITE(sc, FFB_DAC_VALUE, eie);
	DAC_WRITE(sc, FFB_DAC_VALUE, eis);

	FBC_WRITE(sc, FFB_FBC_FBCFG0, fbcfg0);

	DAC_WRITE(sc, FFB_DAC_TYPE, FFB_DAC_TGC);
	DAC_WRITE(sc, FFB_DAC_VALUE, tgc);
	DPRINTF(("new tgc: %08x\n", tgc));
    
	*hres = mode->hdisplay;
	*vres = mode->vdisplay;

	printf("%s: video mode set to %d x %d @ %dHz\n",
	    device_xname(sc->sc_dev),
	    mode->hdisplay, mode->vdisplay,
	    DIVIDE(DIVIDE(mode->dot_clock * 1000,
	    mode->htotal), mode->vtotal));

	return 1;
}
