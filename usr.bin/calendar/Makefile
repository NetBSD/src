#	from: @(#)Makefile	4.8 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.4 1993/10/01 00:35:19 jtc Exp $

PROG=	calendar

beforeinstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}/usr/share/calendar
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/calendars/calendar.* ${DESTDIR}/usr/share/calendar

.include <bsd.prog.mk>
