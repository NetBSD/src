#	$NetBSD: Makefile,v 1.7 1996/10/18 06:12:59 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/17/94

PROG=	compress
SRCS=	compress.c zopen.c
LINKS=	${BINDIR}/compress ${BINDIR}/uncompress
MLINKS= compress.1 uncompress.1
#MLINKS+=compress.1 zcat.1

#afterinstall:
#	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
#	    ${.CURDIR}/zcat.sh ${DESTDIR}/usr/bin/zcat

.include <bsd.prog.mk>
