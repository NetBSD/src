#	$NetBSD: Makefile,v 1.4 1996/10/18 06:29:29 thorpej Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:47 vixie Exp 

.PATH:	${.CURDIR}/../man

all:	named.restart
CLEANFILES+= named.restart
MAN=	named.restart.8

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    named.restart ${DESTDIR}${BINDIR}/named.restart

named.restart:	named.restart.sh Makefile ${.CURDIR}/../Makefile.inc
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    < ${.CURDIR}/named.restart.sh > named.restart
	chmod +x named.restart

.include "../../Makefile.inc"
.include <bsd.prog.mk>
