#	$NetBSD: Makefile,v 1.4 1996/10/18 06:29:11 thorpej Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:46 vixie Exp 

.PATH:	${.CURDIR}/../man

all:	named.reload
CLEANFILES+= named.reload
MAN=	named.reload.8

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    named.reload ${DESTDIR}${BINDIR}/named.reload

named.reload:	named.reload.sh Makefile ${.CURDIR}/../Makefile.inc
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    < ${.CURDIR}/named.reload.sh > named.reload
	chmod +x named.reload

.include "../../Makefile.inc"
.include <bsd.prog.mk>
