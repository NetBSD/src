#	$NetBSD: Makefile,v 1.3.4.1 2002/01/10 19:43:14 thorpej Exp $

NOMAN=	# defined

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${INSTPRIV} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
