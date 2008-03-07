#	$NetBSD: Makefile,v 1.13 2008/03/07 19:22:22 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	calendar
.if ${MKSHARE} != "no"
FILESDIR=/usr/share/calendar
FILES!= echo ${.CURDIR}/calendars/calendar.*
.endif
WARNS=4

.include <bsd.prog.mk>
