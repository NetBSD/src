/*	$NetBSD: ntp_debug.h,v 1.4.14.1 2018/09/30 01:45:14 pgoyette Exp $	*/

/*
 * Header
 *
 * Created
 *
 * Copyright (C) 2005 by Frank Kardel
 */
#ifndef NTP_DEBUG_H
#define NTP_DEBUG_H

/*
 * macro for debugging output - cut down on #ifdef pollution.
 *
 * TRACE() is similar to ntpd's DPRINTF() for utilities and libntp.
 * Uses mprintf() and so supports %m, replaced by strerror(errno).
 *
 * The calling convention is not attractive:
 *     TRACE(debuglevel, (fmt, ...));
 *     TRACE(2, ("this will appear on stdout if debug >= %d\n", 2));
 */
#define TRACE(lvl, arg)					\
	do { 						\
		if (debug >= (lvl))			\
			mprintf arg;			\
	} while (0)

#endif	/* NTP_DEBUG_H */
