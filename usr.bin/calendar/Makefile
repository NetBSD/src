#	$NetBSD: Makefile,v 1.7 1996/10/18 06:12:29 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	calendar

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/calendars/calendar.* ${DESTDIR}/usr/share/calendar

.include <bsd.prog.mk>
