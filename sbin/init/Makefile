#	$NetBSD: Makefile,v 1.18 1997/07/19 19:00:58 perry Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500
INSTALLFLAGS=-fschg
CFLAGS+=-DALTSHELL -DSECURE
WARNS=1

.include <bsd.prog.mk>
