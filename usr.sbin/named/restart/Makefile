#	$NetBSD: Makefile,v 1.6 1997/04/07 07:56:24 lukem Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:47 vixie Exp 

.PATH:	${.CURDIR}/../man

all:	named.restart
CLEANFILES+= named.restart
MAN=	named.restart.8
FILES=	named.restart

.include "../../Makefile.inc"

named.restart:	named.restart.sh Makefile ${.CURDIR}/../Makefile.inc
	rm -f $@
	sed -e "s|%DESTSBIN%|${BINDIR}|" \
	    < ${.CURDIR}/named.restart.sh > $@
	chmod +x $@

.include <bsd.prog.mk>
