#	from: @(#)Makefile	4.8 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.5 1994/02/10 02:03:28 cgd Exp $

PROG=	calendar

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/calendars/calendar.* ${DESTDIR}/usr/share/calendar

.include <bsd.prog.mk>
