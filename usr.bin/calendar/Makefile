#	$NetBSD: Makefile,v 1.10 1999/02/13 02:54:54 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	calendar
.if ${MKSHARE} != "no"
FILESDIR=/usr/share/calendar
FILES!= echo ${.CURDIR}/calendars/calendar.*
.endif

.include <bsd.prog.mk>
