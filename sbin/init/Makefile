#	$NetBSD: Makefile,v 1.26 2002/01/21 15:57:40 abs Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
CPPFLAGS+=-DALTSHELL -DSECURE -DMFS_DEV_IF_NO_CONSOLE

.include <bsd.prog.mk>

LDSTATIC=-static
