#	$NetBSD: Makefile,v 1.4 1996/10/18 06:28:24 thorpej Exp $
# from: $Id: Makefile,v 8.1 1994/12/15 06:23:47 vixie Exp 

.PATH:	${.CURDIR}/../man

all:	ndc
CLEANFILES+= ndc
MAN=	ndc.8

realinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ndc ${DESTDIR}${BINDIR}/ndc

ndc: ndc.sh Makefile ${.CURDIR}/../Makefile.inc
	sed -e "s|%PIDDIR%|${PIDDIR}|" \
	    -e "s|%PS%|${PS}|" \
	    -e "s|%DESTSBIN%|${BINDIR}|" \
	    -e "s|%IOT%|${IOT}|" \
	    < ${.CURDIR}/ndc.sh > ndc
	chmod +x ndc

.include "../../Makefile.inc"
.include <bsd.prog.mk>
