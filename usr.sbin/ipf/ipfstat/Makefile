#	$NetBSD: Makefile,v 1.20 2010/02/06 23:45:26 he Exp $

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES} ${LIBTERMINFO}
LDADD+=  	-lcurses -lterminfo

.include <bsd.prog.mk>
