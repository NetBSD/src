#	$NetBSD: Makefile,v 1.14.6.1 2000/06/23 16:39:52 minoura Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c

BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg

# XXX Kerberos support broken right now.

#SRCS+=	kcmd.c
#CPPFLAGS+=-DKERBEROS -DCRYPT

#DPADD+= ${LIBKSTREAM} ${LIBKRB} ${LIBDES} ${LIBCOM_ERR}
#LDADD+= -lkstream -lkrb -ldes -lcom_err

.include <bsd.prog.mk>
