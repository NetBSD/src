#	$NetBSD: Makefile,v 1.1 2012/03/23 21:29:45 christos Exp $

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES} ${LIBTERMINFO}
LDADD+=  	-lcurses -lterminfo

.include <bsd.prog.mk>
