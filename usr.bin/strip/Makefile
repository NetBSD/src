#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.9 1994/06/02 23:40:17 jtc Exp $

PROG=	strip

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} strip \
	    ${DESTDIR}${BINDIR}
	./strip ${DESTDIR}${BINDIR}/strip
	-if [ "${COPY}" = "" ]; then rm -f ./strip; fi

.include <bsd.prog.mk>
