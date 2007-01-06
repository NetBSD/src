/*	$NetBSD: ntp_debug.h,v 1.1.1.1 2007/01/06 16:05:45 kardel Exp $	*/

/*
 * $Header: /cvsroot/src/dist/ntp/include/Attic/ntp_debug.h,v 1.1.1.1 2007/01/06 16:05:45 kardel Exp $
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
/*
 * $Log: ntp_debug.h,v $
 * Revision 1.1.1.1  2007/01/06 16:05:45  kardel
 * Import ntp 4.2.4
 *
 */
