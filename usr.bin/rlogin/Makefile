#	$NetBSD: Makefile,v 1.7 1996/01/04 22:32:15 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c
#SRCS+=	krcmd.c kcmd.c des_rw.c
#DPADD=	${LIBKRB} ${LIBDES}
#CFLAGS+=-DKERBEROS -DCRYPT
#LDADD=	-lkrb -ldes
BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg

.include <bsd.prog.mk>
