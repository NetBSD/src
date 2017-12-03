/* $NetBSD: wsbelldata.h,v 1.2.6.2 2017/12/03 11:37:37 jdolecek Exp $ */
/*-
 * Copyright (c) 2017 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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

#ifndef WSKBD_DEFAULT_BELL_PITCH
#define	WSKBD_DEFAULT_BELL_PITCH	1500	/* 1500Hz */
#endif
#ifndef WSKBD_DEFAULT_BELL_PERIOD
#define	WSKBD_DEFAULT_BELL_PERIOD	100	/* 100ms */
#endif
#ifndef WSKBD_DEFAULT_BELL_VOLUME
#define	WSKBD_DEFAULT_BELL_VOLUME	50	/* 50% volume */
#endif

#define	SETBELL(dstp, srcp, dfltp)					\
    do {								\
	(dstp)->pitch = ((srcp)->which & WSKBD_BELL_DOPITCH) ?		\
	    (srcp)->pitch : (dfltp)->pitch;				\
	(dstp)->period = ((srcp)->which & WSKBD_BELL_DOPERIOD) ?	\
	    (srcp)->period : (dfltp)->period;				\
	(dstp)->volume = ((srcp)->which & WSKBD_BELL_DOVOLUME) ?	\
	    (srcp)->volume : (dfltp)->volume;				\
	(dstp)->which = WSKBD_BELL_DOALL;				\
    } while (0)

static struct wskbd_bell_data wskbd_default_bell_data = {
	WSKBD_BELL_DOALL,
	WSKBD_DEFAULT_BELL_PITCH,
	WSKBD_DEFAULT_BELL_PERIOD,
	WSKBD_DEFAULT_BELL_VOLUME,
};
