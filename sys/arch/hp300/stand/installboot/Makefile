#	$NetBSD: Makefile,v 1.2.8.1 2000/11/20 20:08:10 bouyer Exp $

MKMAN=	no

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
