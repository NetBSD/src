#	$NetBSD: Makefile,v 1.11 1997/10/24 09:00:58 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c

.if defined(KERBEROS)
SRCS+=	kcmd.c
DPADD=	${LIBKSTREAM} ${LIBKRB} ${LIBDES}
CPPFLAGS+= -DKERBEROS -DCRYPT
LDADD=	-lkstream -lkrb -ldes
.endif
BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg

.include <bsd.prog.mk>
