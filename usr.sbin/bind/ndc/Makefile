#	$NetBSD: Makefile,v 1.1.1.1 1998/10/05 18:02:00 tron Exp $

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
	sed -e "s|%NAMED%|${NAMED}|" \
	    -e "s|%PIDFILE%|${PIDFILE}|" \
	    -e "s|%PS%|${PS}|" \
	    -e "s|:/usr/ucb||" \
	    < ${.CURDIR}/ndc.sh > $@
	chmod +x $@

.include <bsd.prog.mk>
