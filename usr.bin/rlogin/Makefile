#	$NetBSD: Makefile,v 1.18 2004/09/05 08:38:23 manu Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c
WARNS=	2

BINOWN=	root
BINMODE=4555

# XXX Kerberos support broken right now.
# XXX Unsupported Kerberos options were removed from man page
# XXX Don't forget to update the man page if you fix Kerberos

#SRCS+=	kcmd.c
#CPPFLAGS+=-DKERBEROS -DCRYPT

#DPADD+= ${LIBKSTREAM} ${LIBKRB} ${LIBDES} ${LIBCOM_ERR}
#LDADD+= -lkstream -lkrb -ldes -lcom_err

.include <bsd.prog.mk>
