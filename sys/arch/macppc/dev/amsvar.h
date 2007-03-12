/*	$NetBSD: amsvar.h,v 1.6.2.1 2007/03/12 05:49:05 rmind Exp $	*/

/*
 * Copyright (C) 1998	Colin Wood
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
 *	This product includes software developed by Colin Wood.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACPPC_AMSVAR_H_
#define _MACPPC_AMSVAR_H_

/*
 * State info, per mouse instance.
 */
struct ams_softc {
	struct	device	sc_dev;

	/* ADB info */
	int		origaddr;	/* ADB device type (ADBADDR_MS) */
	int		adbaddr;	/* current ADB address */
	int		handler_id;	/* type of mouse */

	/* Extended Mouse Protocol info, faked for non-EMP mice */
	u_int8_t	sc_class;	/* mouse class (mouse, trackball) */
	u_int8_t	sc_buttons;	/* number of buttons */
	u_int32_t	sc_res;		/* mouse resolution (dpi) */
	char		sc_devid[5];	/* device indentifier */

	int		sc_mb;		/* current button state */
	struct device	*sc_wsmousedev;
	/* helpers for trackpads */
	int		sc_down;
	int		sc_tapping;	/* 1 - tapping causes button event */
	/*
	 * trackpad protocol variant. Known so far:
	 * 2 buttons - PowerBook 3400, single events on button 3 and 4 indicate
	 *             finger down and up
	 * 4 buttons - iBook G4, button 6 indicates finger down, button 4 is
	 *             always down
	 */
	int		sc_x, sc_y;
};

/* EMP device classes */
#define MSCLASS_TABLET		0
#define MSCLASS_MOUSE		1
#define MSCLASS_TRACKBALL	2
#define MSCLASS_TRACKPAD	3

void ms_adbcomplete __P((uint8_t *buffer, uint8_t *data_area, int adb_command));
void ms_handoff __P((adb_event_t *event, struct ams_softc *));

#endif /* _MACPPC_AMSVAR_H_ */
