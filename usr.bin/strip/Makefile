#	$NetBSD: Makefile,v 1.11 1996/10/08 22:00:18 christos Exp $
#	@(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	strip

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 600 strip \
	    ${DESTDIR}${BINDIR}
	./strip ${DESTDIR}${BINDIR}/strip
	chmod ${BINMODE} ${DESTDIR}${BINDIR}/strip
	-if [ "${COPY}" = "" ]; then rm -f ./strip; fi

.include <bsd.prog.mk>
