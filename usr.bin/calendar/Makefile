#	$NetBSD: Makefile,v 1.13.10.1 2009/05/13 19:19:45 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	calendar
.if ${MKSHARE} != "no"
FILESDIR=/usr/share/calendar
FILES!= echo ${.CURDIR}/calendars/calendar.*
.endif

.include <bsd.prog.mk>
