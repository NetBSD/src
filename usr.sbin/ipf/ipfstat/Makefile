#	$NetBSD: Makefile,v 1.16.2.2 2004/04/05 20:41:07 tron Exp $

NOGCCERROR=	# defined

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES}
LDADD+=  	-lcurses

.include <bsd.prog.mk>
