#	$NetBSD: Makefile,v 1.12 1996/10/18 06:20:54 thorpej Exp $
#	@(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	strip

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 strip \
	    ${DESTDIR}${BINDIR}
	./strip ${DESTDIR}${BINDIR}/strip
	chmod ${BINMODE} ${DESTDIR}${BINDIR}/strip
	-if [ "${COPY}" = "" ]; then rm -f ./strip; fi

.include <bsd.prog.mk>
