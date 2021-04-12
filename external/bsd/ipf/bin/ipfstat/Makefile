#	$NetBSD: Makefile,v 1.2 2021/04/12 02:54:07 mrg Exp $

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES} ${LIBTERMINFO}
LDADD+=  	-lcurses -lterminfo

COPTS.ipfstat.c+=	${GCC_NO_FORMAT_OVERFLOW}

.include <bsd.prog.mk>
