#	$NetBSD: Makefile,v 1.5 1997/01/09 20:23:23 tls Exp $
#	from: @(#)Makefile	5.5 (Berkeley) 7/1/90

MAN=	which.1

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/which.csh ${DESTDIR}${BINDIR}/which

.include <bsd.prog.mk>
