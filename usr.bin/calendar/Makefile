#	@(#)Makefile	4.8 (Berkeley) 5/11/90

PROG=	calendar

beforeinstall:
	@if [ ! -d ${DESTDIR}/usr/share/calendar ]; then \
                /bin/rm -f ${DESTDIR}/usr/share/calendar ; \
                mkdir -p ${DESTDIR}/usr/share/calendar ; \
                chown root.wheel ${DESTDIR}/usr/share/calendar ; \
                chmod 755 ${DESTDIR}/usr/share/calendar ; \
        else \
                true ; \
        fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    ${.CURDIR}/calendars/calendar.* ${DESTDIR}/usr/share/calendar

.include <bsd.prog.mk>
