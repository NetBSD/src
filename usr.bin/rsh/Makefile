#	$NetBSD: Makefile,v 1.4 1997/02/16 15:01:02 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rsh
#CFLAGS+=-DKERBEROS -DCRYPT
#SRCS=	rsh.c krcmd.c kcmd.c des_rw.c
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg
.PATH:	${.CURDIR}/../rlogin

.include <bsd.prog.mk>
