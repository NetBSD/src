/* 	$NetBSD: tftvar.h,v 1.1.8.1 2007/03/12 05:47:40 rmind Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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

#ifndef	_VIRTEX_DEV_TFTVAR_H_
#define	_VIRTEX_DEV_TFTVAR_H_

struct tft_softc {
	struct device 		sc_dev;

	void *		sc_image;
	size_t 			sc_size;

	bus_space_tag_t 	sc_iot;
	bus_space_handle_t 	sc_ioh;

	u_int 			sc_width;
	u_int 			sc_height;
	u_int 			sc_stride; 	/* line length in bytes */
	u_int 			sc_bpp; 	/* bits per pixel */

	void 			*sc_sdhook;

	/* wscons */
	struct wsscreen_descr 	sc_ws_descr_storage[1];
	struct wsscreen_descr 	*sc_ws_descr; 	/* Fixed resolution */
	struct wsscreen_list 	sc_ws_scrlist;
	struct vcons_screen 	sc_vc_screen;
	struct vcons_data 	sc_vc_data;

	/* splashscreen */
#ifdef SPLASHSCREEN
	struct splash_info 	sc_sp_info;
#endif
#ifdef SPLASHSCREEN_PROGRESS
	struct splash_progress 	sc_sp_progress;
#endif
};

void 		tft_attach(device_t, struct wsdisplay_accessops *);
void 		tft_shutdown(void *);
int 		tft_ioctl(void *, void *, u_long, void *, int, struct lwp *);
int 		tft_mode(device_t);

#endif	/*_VIRTEX_DEV_TFTVAR_H_*/
