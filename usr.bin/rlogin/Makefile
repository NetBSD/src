#	$NetBSD: Makefile,v 1.9 1997/02/21 07:52:37 mikel Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c

.if defined(KERBEROS)
SRCS+=	krcmd.c kcmd.c
DPADD=	${LIBKSTREAM} ${LIBKRB} ${LIBDES}
CFLAGS+= -DKERBEROS -DCRYPT
LDADD=	-lkstream -lkrb -ldes
.endif
BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg

.include <bsd.prog.mk>
