#	$NetBSD: Makefile,v 1.17 2004/03/29 04:40:01 lukem Exp $

NOGCCERROR=	# defined

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES} ${LIBKVM}
LDADD+=  	-lcurses -lkvm

.include <bsd.prog.mk>
