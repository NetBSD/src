#	$NetBSD: Makefile,v 1.5 1994/12/21 08:42:01 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

MAN1=	shar.0

NOPROG=noprog

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/shar.sh ${DESTDIR}/usr/bin/shar

.include <bsd.prog.mk>
