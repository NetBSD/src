#	$NetBSD: Makefile,v 1.23.4.2 2002/02/13 21:57:51 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
CPPFLAGS+=-DALTSHELL -DSECURE -DMFS_DEV_IF_NO_CONSOLE

.include <bsd.prog.mk>
