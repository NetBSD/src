#	$NetBSD: Makefile,v 1.7 1994/12/23 01:13:58 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

MAN=	lorder.1

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/lorder.sh ${DESTDIR}${BINDIR}/lorder

.include <bsd.prog.mk>
