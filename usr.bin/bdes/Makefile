#	$NetBSD: Makefile,v 1.2 1996/10/18 06:12:01 thorpej Exp $

MAN=	bdes.1

SCRIPT=	bdes.sh

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/${SCRIPT} ${DESTDIR}${BINDIR}/bdes

.include <bsd.prog.mk>
