#	@(#)Makefile	5.5 (Berkeley) 6/29/90

PROG=	inetd
MAN8=	inetd.0
MLINKS=	inetd.8 inetd.5

DPADD=	${LIBRPC} ${LIBUTIL}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
