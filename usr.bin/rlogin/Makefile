#	$NetBSD: Makefile,v 1.16 2000/10/18 00:24:19 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c

BINOWN=	root
BINMODE=4555

# XXX Kerberos support broken right now.

#SRCS+=	kcmd.c
#CPPFLAGS+=-DKERBEROS -DCRYPT

#DPADD+= ${LIBKSTREAM} ${LIBKRB} ${LIBDES} ${LIBCOM_ERR}
#LDADD+= -lkstream -lkrb -ldes -lcom_err

.include <bsd.prog.mk>
