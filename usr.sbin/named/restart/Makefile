#	$NetBSD: Makefile,v 1.9 1997/05/06 20:46:35 gwr Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:47 vixie Exp 

.PATH.c: ${.CURDIR}/../man

all:	named.restart
CLEANFILES+= named.restart
MAN=	named.restart.8
FILES=	named.restart
FILESMODE= ${BINMODE}

.include "../../Makefile.inc"

named.restart:	named.restart.sh Makefile ${.CURDIR}/../Makefile.inc
	rm -f $@
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    -e "s|%INDOT%||" \
	    < ${.CURDIR}/named.restart.sh > $@
	chmod +x $@

.include <bsd.prog.mk>
