#	$NetBSD: Makefile,v 1.7 1997/05/06 20:45:39 gwr Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rsh
BINOWN=	root
BINMODE=4555
#CFLAGS+=-DKERBEROS -DCRYPT
#SRCS=	rsh.c krcmd.c kcmd.c des_rw.c
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
#INSTALLFLAGS=-fschg
.PATH.c: ${.CURDIR}/../rlogin

.include <bsd.prog.mk>
