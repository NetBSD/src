#	$NetBSD: Makefile,v 1.5 1997/03/24 22:02:54 christos Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:46 vixie Exp 

.PATH:	${.CURDIR}/../man

all:	named.reload
CLEANFILES+= named.reload
MAN=	named.reload.8
FILES=named.reload
FILESMODE=${BINMODE}

named.reload:	named.reload.sh Makefile ${.CURDIR}/../Makefile.inc
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    < ${.CURDIR}/named.reload.sh > named.reload
	chmod +x named.reload

.include "../../Makefile.inc"
.include <bsd.prog.mk>
