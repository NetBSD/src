#	$NetBSD: Makefile,v 1.6 1997/04/07 07:48:53 lukem Exp $
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
	rm ndc
	sed -e "s|%PIDDIR%|${PIDDIR}|" \
	    -e "s|%PS%|${PS}|" \
	    -e "s|%DESTSBIN%|${BINDIR}|" \
	    -e "s|%IOT%|${IOT}|" \
	    < ${.CURDIR}/ndc.sh > ndc
	chmod +x ndc

.include <bsd.prog.mk>
