#	$NetBSD: Makefile,v 1.3 2023/06/03 09:09:03 lukem Exp $

PROG=		ipfstat
SRCS=		ipfstat.c
MAN=		ipfstat.8
DPADD+=  	${LIBCURSES} ${LIBTERMINFO}
LDADD+=  	-lcurses -lterminfo

COPTS.ipfstat.c+=	${CC_WNO_FORMAT_OVERFLOW}

.include <bsd.prog.mk>
