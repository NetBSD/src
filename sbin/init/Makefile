#	$NetBSD: Makefile,v 1.23 2000/03/19 23:22:55 soren Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
INSTALLFLAGS=-fschg
CPPFLAGS+=-DALTSHELL -DSECURE -DMSDOSFS_ROOT

.include <bsd.prog.mk>
