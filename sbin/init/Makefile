#	$NetBSD: Makefile,v 1.21 1997/10/10 19:48:52 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500
INSTALLFLAGS=-fschg
CPPFLAGS+=-DALTSHELL -DSECURE -DMSDOSFS_ROOT

.include <bsd.prog.mk>
