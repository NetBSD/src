#	$NetBSD: Makefile,v 1.12.30.1 2008/03/24 07:16:34 keiichi Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	calendar
.if ${MKSHARE} != "no"
FILESDIR=/usr/share/calendar
FILES!= echo ${.CURDIR}/calendars/calendar.*
.endif
WARNS=4

.include <bsd.prog.mk>
