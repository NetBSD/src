#	$NetBSD: Makefile,v 1.17 1997/07/19 18:12:47 perry Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500
INSTALLFLAGS=-fschg
CFLAGS+=-DALTSHELL -DSECURE

.include <bsd.prog.mk>
