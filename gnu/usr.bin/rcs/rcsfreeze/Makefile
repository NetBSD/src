#	$NetBSD: Makefile,v 1.1.1.2 1996/10/13 21:57:48 veego Exp $

MAN=	rcsfreeze.1

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/rcsfreeze.sh ${DESTDIR}${BINDIR}/rcsfreeze

.include "../../Makefile.inc"
.include <bsd.prog.mk>
