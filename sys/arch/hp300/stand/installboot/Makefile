#	$NetBSD: Makefile,v 1.4.2.3 2002/02/28 04:09:32 nathanw Exp $

NOMAN=	# defined

beforeinstall:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
