#	$NetBSD: Makefile,v 1.7 1996/10/18 06:19:28 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

MAN=	shar.1

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/shar.sh ${DESTDIR}/usr/bin/shar

.include <bsd.prog.mk>
