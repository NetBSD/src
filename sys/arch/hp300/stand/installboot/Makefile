#	$NetBSD: Makefile,v 1.6 2002/02/09 09:36:00 lukem Exp $

NOMAN=	# defined

beforeinstall:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
