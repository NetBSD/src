#	@(#)Makefile	8.2 (Berkeley) 4/3/94
#	$NetBSD: Makefile,v 1.3 1995/08/31 23:38:49 jtc Exp $

MAN1=	pagesize.0

all pagesize: ${MAN1}

clean depend lint tags:

cleandir:
	rm -f ${MAN1}

install: maninstall
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/pagesize.sh ${DESTDIR}/${BINDIR}/pagesize

.include <bsd.prog.mk>
.include <bsd.man.mk>
