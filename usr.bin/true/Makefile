#	$NetBSD: Makefile,v 1.8 1997/01/09 20:23:09 tls Exp $
#	from: @(#)Makefile	5.3 (Berkeley) 5/11/90

MAN=	true.1

beforeinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/true.sh ${DESTDIR}${BINDIR}/true

.include <bsd.prog.mk>
