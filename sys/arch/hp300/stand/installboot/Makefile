#	$NetBSD: Makefile,v 1.3 2000/09/23 13:53:41 simonb Exp $

MKMAN=	no

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
