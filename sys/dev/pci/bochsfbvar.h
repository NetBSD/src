/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BOCHSFBVAR_H
#define BOCHSFBVAR_H

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/videomode/edidvar.h>

#ifndef BOCHSFB_DEFAULT_WIDTH
#define BOCHSFB_DEFAULT_WIDTH	(1024)
#endif

#ifndef BOCHSFB_DEFAULT_HEIGHT
#define BOCHSFB_DEFAULT_HEIGHT	(768)
#endif

/* Structure for the Bochs FB driver */
struct bochsfb_softc {
	device_t sc_dev;

	/* PCI attachment */
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;

	/* Bus space tags and handles */
	bus_space_tag_t sc_iot;          /* I/O space tag */
	bus_space_handle_t sc_ioh_vga;   /* VGA I/O handle */
	bus_space_handle_t sc_ioh_dispi; /* DISPI I/O handle */

	bus_space_tag_t sc_mmiot;     /* MMIO space tag */
	bus_space_handle_t sc_mmioh;  /* MMIO handle for DISPI interface */

	bus_space_tag_t sc_memt;      /* Memory space tag */
	bus_space_handle_t sc_fb_handle; /* Framebuffer handle */

	bus_addr_t sc_mmio_addr;      /* MMIO base address */
	bus_size_t sc_mmio_size;      /* MMIO size */

	bus_addr_t sc_fb_addr;        /* Framebuffer physical address */
	bus_size_t sc_fb_size;        /* Framebuffer size */

	bool sc_has_mmio;             /* Whether MMIO is available */

	/* Device Info */
	uint16_t sc_id;	  /* Device ID */
	pcireg_t sc_pci_id; /* PCI ID */

	/* Video mode parameters */
	int sc_width, sc_height, sc_linebytes, sc_bpp;
	const struct videomode *sc_videomode;

	uint8_t edid_buf[0x400]; /* EDID data buffer */
	struct edid_info sc_ei;

	/* WS Display data */
	int sc_blank;
	int sc_mode;
	struct vcons_screen sc_console_screen;
	struct vcons_data vd;
	struct wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct wsscreen_list sc_screenlist;
};
 
#endif /* BOCHSFBVAR_H */
