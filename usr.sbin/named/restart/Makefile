#	$NetBSD: Makefile,v 1.5 1997/03/24 22:02:56 christos Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:47 vixie Exp 

.PATH:	${.CURDIR}/../man

all:	named.restart
CLEANFILES+= named.restart
MAN=	named.restart.8
FILES=	named.restart

.include "../../Makefile.inc"

named.restart:	named.restart.sh Makefile ${.CURDIR}/../Makefile.inc
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    < ${.CURDIR}/named.restart.sh > named.restart
	chmod +x named.restart

.include <bsd.prog.mk>
