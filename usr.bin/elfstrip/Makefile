#	$NetBSD: Makefile,v 1.4 1997/01/09 20:18:38 tls Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	elfstrip

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 ${PROG} \
	    ${DESTDIR}${BINDIR}
	./${PROG} ${DESTDIR}${BINDIR}/${PROG}
	chmod ${BINMODE} ${DESTDIR}${BINDIR}/${PROG}
	-if [ "${COPY}" = "" ]; then rm -f ./${PROG}; fi

.include <bsd.prog.mk>
