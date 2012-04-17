#	$NetBSD: Makefile,v 1.1.2.2 2012/04/17 00:03:10 yamt Exp $

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES} ${LIBTERMINFO}
LDADD+=  	-lcurses -lterminfo

.include <bsd.prog.mk>
