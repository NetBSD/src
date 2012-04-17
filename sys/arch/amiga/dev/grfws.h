/*	$NetBSD: grfws.h,v 1.1.6.2 2012/04/17 00:06:02 yamt Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Wille and Radoslaw Kujawa.
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

#ifndef _AMIGA_GRFWS_H_
#if NWSDISPLAY > 0 

/*
 * Structure holding pointers to hw-dependent implementations of access op.
 * This struct should be filled during driver initialization and later used 
 * in wsdisplay part of grf.c.
 */
struct ws_ao_ioctl {
	int	(*ginfo)(void *, void *);
	int	(*getcmap)(void *, void *);
	int	(*putcmap)(void *, void *);
	int	(*gvideo)(void *, void *);
	int	(*svideo)(void *, void *);
	int	(*gmode)(void *, void *);
	int 	(*smode)(void *, void *);
	int	(*gtype)(void *, void *);	
};

/*
 * Generic wsdisplay access ops that should be used from most grf drivers
 * (if running in wscons mode, not ite).
 *
 * Implementation in grf.c.
 */
paddr_t	grf_wsmmap(void *, void *, off_t, int);
int	grf_wsioctl(void *, void *, u_long, void *, int, struct lwp *);

/*
 * Generic access ops ioctls.
 */
int	grf_wsaoginfo(void *, void *);
int	grf_wsaogetcmap(void *, void *);
int	grf_wsaoputcmap(void *, void *);
int	grf_wsaogvideo(void *, void *);
int	grf_wsaosvideo(void *, void *);
int	grf_wsaogmode(void *, void *);
int	grf_wsaosmode(void *, void *);
int	grf_wsaogtype(void *, void *);

#endif /* NWSDISPLAY > 0 */
#endif /* _AMIGA_GRFWS_H_ */
