#	@(#)Makefile	8.2 (Berkeley) 4/3/94
#	$NetBSD: Makefile,v 1.5 1995/09/05 01:06:38 tls Exp $

MAN=	pagesize.1

all pagesize: ${MAN1}

clean depend lint tags:

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/pagesize.sh ${DESTDIR}/${BINDIR}/pagesize

.include <bsd.prog.mk>
.include <bsd.man.mk>
