#	$NetBSD: Makefile,v 1.4.2.2 2002/01/08 00:24:47 nathanw Exp $

NOMAN=	# defined

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${INSTPRIV} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
