#	$NetBSD: Makefile,v 1.10 1997/06/28 23:43:36 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c

.if defined(KERBEROS)
SRCS+=	kcmd.c
DPADD=	${LIBKSTREAM} ${LIBKRB} ${LIBDES}
CFLAGS+= -DKERBEROS -DCRYPT
LDADD=	-lkstream -lkrb -ldes
.endif
BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg

.include <bsd.prog.mk>
