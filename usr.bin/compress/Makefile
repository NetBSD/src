#	from: @(#)Makefile	8.2 (Berkeley) 4/17/94
#	$Id: Makefile,v 1.4.2.1 1994/10/17 21:48:19 cgd Exp $

PROG=	compress
SRCS=	compress.c zopen.c
LINKS=	${BINDIR}/compress ${BINDIR}/uncompress
MLINKS= compress.1 uncompress.1
#MLINKS+=compress.1 zcat.1

#afterinstall:
#	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
#	    ${.CURDIR}/zcat.sh ${DESTDIR}/usr/bin/zcat

.include <bsd.prog.mk>
