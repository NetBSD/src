#	$NetBSD: Makefile,v 1.24 2000/10/18 00:24:20 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
CPPFLAGS+=-DALTSHELL -DSECURE -DMSDOSFS_ROOT

.include <bsd.prog.mk>
