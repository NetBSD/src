#	$NetBSD: Makefile,v 1.2 1999/02/13 02:54:44 lukem Exp $

MKMAN=	no

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
