#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.2 1996/09/29 22:23:03 jonathan Exp $

PROG=	elfstrip

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 600 ${PROG} \
	    ${DESTDIR}${BINDIR}
	./${PROG} ${DESTDIR}${BINDIR}/${PROG}
	chmod ${BINMODE} ${DESTDIR}${BINDIR}/${PROG}
	-if [ "${COPY}" = "" ]; then rm -f ./${PROG}; fi

.include <bsd.prog.mk>
