#	$NetBSD: Makefile,v 1.9 1998/09/27 17:22:10 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	calendar
.if !defined (NOSHARE)
FILESDIR=/usr/share/calendar
FILES!= echo ${.CURDIR}/calendars/calendar.*
.endif

.include <bsd.prog.mk>
