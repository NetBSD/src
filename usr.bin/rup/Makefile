#
# a makefile for /usr/bin/rup
#
# $Header: /cvsroot/src/usr.bin/rup/Makefile,v 1.1 1993/06/03 19:32:28 brezak Exp $
#

PROG = rup
MAN1=	rup.0

DPADD=	${LIBRPCSVC} ${LIBRPC}
LDADD=	${LDARGS} -lrpcsvc -lrpc

.include <bsd.prog.mk>
