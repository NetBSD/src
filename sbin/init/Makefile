#	$NetBSD: Makefile,v 1.22 1997/10/11 11:05:28 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=544
INSTALLFLAGS=-fschg
CPPFLAGS+=-DALTSHELL -DSECURE -DMSDOSFS_ROOT

.include <bsd.prog.mk>
