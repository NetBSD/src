#	$NetBSD: Makefile,v 1.16 2004/03/28 09:01:27 martti Exp $

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES} ${LIBKVM}
LDADD+=  	-lcurses -lkvm -lipf

NOGCCERROR=	# defined

.include <bsd.prog.mk>
