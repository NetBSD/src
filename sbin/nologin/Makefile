#	$NetBSD: Makefile,v 1.5 1996/10/18 05:51:37 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/22/94

MAN=	nologin.8

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/nologin.sh ${DESTDIR}/sbin/nologin

.include <bsd.prog.mk>
