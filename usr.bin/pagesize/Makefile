#	@(#)Makefile	8.2 (Berkeley) 4/3/94
#	$NetBSD: Makefile,v 1.4 1995/09/01 23:56:34 thorpej Exp $

MAN=	pagesize.1

all pagesize: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/pagesize.sh ${DESTDIR}/${BINDIR}/pagesize

.include <bsd.prog.mk>
.include <bsd.man.mk>
