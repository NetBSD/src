#	$NetBSD: Makefile,v 1.4 2001/11/08 07:45:26 gmcgarry Exp $

MKMAN=	no

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${INSTPRIV} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
