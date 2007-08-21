/*	$NetBSD: ntp_debug.h,v 1.2 2007/08/21 13:17:24 ghen Exp $	*/

/*
 * $Header: /cvsroot/src/dist/ntp/include/Attic/ntp_debug.h,v 1.2 2007/08/21 13:17:24 ghen Exp $
 *
 * Created: Sat Aug 20 14:23:01 2005
 *
 * Copyright (C) 2005 by Frank Kardel
 */
#ifndef NTP_DEBUG_H
#define NTP_DEBUG_H

/*
 * macros for debugging output - cut down on #ifdef pollution in the code
 */

#ifdef DEBUG
#define DPRINTF(_lvl_, _arg_)                   \
        if (debug >= (_lvl_))                   \
                printf _arg_;
#else
#define DPRINTF(_lvl_, _arg_)
#endif

#endif
