#	$NetBSD: Makefile,v 1.8 1997/05/08 21:11:46 gwr Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rsh
BINOWN=	root
BINMODE=4555
#CFLAGS+=-DKERBEROS -DCRYPT
#SRCS=	rsh.c krcmd.c kcmd.c des_rw.c
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
#INSTALLFLAGS=-fschg
.PATH:	${.CURDIR}/../rlogin

.include <bsd.prog.mk>
