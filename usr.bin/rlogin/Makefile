#	$NetBSD: Makefile,v 1.15 2000/06/20 06:00:38 thorpej Exp $
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
