#	$NetBSD: Makefile,v 1.11 2001/12/04 15:55:32 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	calendar
.if ${MKSHARE} != "no"
FILESDIR=/usr/share/calendar
FILES!= echo ${.CURDIR}/calendars/calendar.*
.endif
DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
