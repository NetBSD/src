#	$NetBSD: Makefile,v 1.3.4.2 2002/03/16 15:57:37 jdolecek Exp $

NOMAN=	# defined

beforeinstall:
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/installboot.sh ${DESTDIR}${BINDIR}/installboot

.include <bsd.prog.mk>
