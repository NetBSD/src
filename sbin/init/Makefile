#	$NetBSD: Makefile,v 1.19 1997/07/30 03:43:19 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500
INSTALLFLAGS=-fschg
CFLAGS+=-DALTSHELL -DSECURE -DMSDOSFS_ROOT
WARNS=1

.include <bsd.prog.mk>
