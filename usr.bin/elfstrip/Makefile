#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.3 1996/10/18 06:21:30 thorpej Exp $

PROG=	elfstrip

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 ${PROG} \
	    ${DESTDIR}${BINDIR}
	./${PROG} ${DESTDIR}${BINDIR}/${PROG}
	chmod ${BINMODE} ${DESTDIR}${BINDIR}/${PROG}
	-if [ "${COPY}" = "" ]; then rm -f ./${PROG}; fi

.include <bsd.prog.mk>
