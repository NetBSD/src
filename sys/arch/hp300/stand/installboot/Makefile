#	$NetBSD: Makefile,v 1.3.6.1 2001/11/12 21:16:58 thorpej Exp $

MKMAN=	no

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${INSTPRIV} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
