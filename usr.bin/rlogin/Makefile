#	$NetBSD: Makefile,v 1.8 1997/01/09 20:21:02 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c
CFLAGS=	-g
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
