#	@(#)Makefile	8.2 (Berkeley) 4/3/94
#	$NetBSD: Makefile,v 1.7 1996/10/18 06:18:22 thorpej Exp $

MAN=	pagesize.1

SCRIPT=	pagesize.sh

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/${SCRIPT} ${DESTDIR}${BINDIR}/pagesize

.include <bsd.prog.mk>
