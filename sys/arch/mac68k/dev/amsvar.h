/*	$NetBSD: amsvar.h,v 1.8.88.1 2012/11/20 03:01:30 tls Exp $	*/

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

#ifndef _MAC68K_MSVAR_H_
#define _MAC68K_MSVAR_H_

/*
 * State info, per mouse instance.
 */
struct ams_softc {
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
	device_t	sc_wsmousedev;
};

/* EMP device classes */
#define MSCLASS_TABLET		0
#define MSCLASS_MOUSE		1
#define MSCLASS_TRACKBALL	2       

void	ms_adbcomplete(void *, void *, int);
void	ams_handoff(adb_event_t *, struct ams_softc *);

#endif /* _MAC68K_MSVAR_H_ */
