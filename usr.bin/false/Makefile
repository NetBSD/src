#	$NetBSD: Makefile,v 1.8 1997/01/09 20:18:42 tls Exp $
#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90

MAN=	false.1

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/false.sh ${DESTDIR}${BINDIR}/false

.include <bsd.prog.mk>
