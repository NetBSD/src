#	$NetBSD: Makefile,v 1.10 1997/05/08 21:12:40 gwr Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:47 vixie Exp 

.PATH:	${.CURDIR}/../man

all:	ndc
CLEANFILES+= ndc
MAN=	ndc.8

.include <bsd.own.mk>
.include "../../Makefile.inc"

FILES=ndc
FILESMODE=${BINMODE}

ndc: ndc.sh Makefile ${.CURDIR}/../Makefile.inc
	rm -f $@
	sed -e "s|%PIDDIR%|${PIDDIR}|" \
	    -e "s|%PS%|${PS}|" \
	    -e "s|%DESTSBIN%|${BINDIR}|" \
	    -e "s|%IOT%|${IOT}|" \
	    -e "s|%INDOT%||" \
	    < ${.CURDIR}/ndc.sh > $@
	chmod +x $@

.include <bsd.prog.mk>
