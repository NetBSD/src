#	$NetBSD: Makefile,v 1.16.2.1 2004/03/29 10:45:07 tron Exp $

NOGCCERROR=	# defined

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES} ${LIBKVM}
LDADD+=  	-lcurses -lkvm

.include <bsd.prog.mk>
