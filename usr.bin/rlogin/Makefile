#	$NetBSD: Makefile,v 1.19 2004/10/16 02:03:54 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rlogin
SRCS=	rlogin.c getport.c
WARNS=	3

.PATH.c: ${.CURDIR}/../rsh
CPPFLAGS+=-I${.CURDIR}/../rsh

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
