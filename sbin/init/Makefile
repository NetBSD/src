#	$NetBSD: Makefile,v 1.23.4.1 2000/10/18 04:18:51 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
CPPFLAGS+=-DALTSHELL -DSECURE -DMSDOSFS_ROOT

.include <bsd.prog.mk>
