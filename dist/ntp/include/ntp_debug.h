/*	$NetBSD: ntp_debug.h,v 1.1.1.1.4.2 2007/08/21 08:39:39 ghen Exp $	*/

/*
 * $Header: /cvsroot/src/dist/ntp/include/Attic/ntp_debug.h,v 1.1.1.1.4.2 2007/08/21 08:39:39 ghen Exp $
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
 * Revision 1.1.1.1.4.2  2007/08/21 08:39:39  ghen
 * Pull up following revision(s) (requested by kardel in ticket #834):
 * 	dist/ntp/*: sync with HEAD as of 20070820
 * 	distrib/sets/lists/base/mi: patch
 * 	distrib/sets/lists/man/mi: patch
 * 	distrib/sets/lists/misc/mi: patch
 * 	etc/mtree/NetBSD.dist: patch
 * 	usr.sbin/ntp/Makefile: revision 1.8-1.9
 * 	usr.sbin/ntp/Makefile.inc: revision 1.16-1.18
 * 	usr.sbin/ntp/html/Makefile: revision 1.1-1.6
 * 	usr.sbin/ntp/include/config.h: revision 1.16-1.17
 * 	usr.sbin/ntp/libopts/Makefile: revision 1.1
 * 	usr.sbin/ntp/ntp-keygen/Makefile: revision 1.2
 * 	usr.sbin/ntp/ntp-keygen/ntp-keygen.8: revision 1.1-1.3
 * 	usr.sbin/ntp/ntpd/Makefile: revision 1.16-1.17
 * 	usr.sbin/ntp/ntpd/ntpd.8: revision 1.15-1.17
 * 	usr.sbin/ntp/ntpdc/Makefile: revision 1.6
 * 	usr.sbin/ntp/ntpdc/ntpdc.8: revision 1.13-1.15
 * 	usr.sbin/ntp/ntpq/Makefile: revision 1.7
 * 	usr.sbin/ntp/ntpq/ntpq.8: revision 1.16-1.18
 * 	usr.sbin/ntp/scripts/mkver: revision 1.6-1.7
 * 	usr.sbin/sntp/Makefile: revision 1.3
 * 	usr.sbin/sntp/sntp.1: revision 1.4
 * Upgrade NTP to version 4.2.4, plus other fixes.
 * Improvements in 4.2.4 (highlights):
 * - dynamic interface handling (no more restarts on wakeup/change in
 *   network connectivity)
 * - faster synchronisation
 * - better refclock support
 *
 * Revision 1.1.1.1  2007/01/06 16:05:45  kardel
 * Import ntp 4.2.4
 *
 */
