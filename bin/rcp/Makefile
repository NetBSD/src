#	$NetBSD: Makefile,v 1.5 1995/03/21 08:18:58 cgd Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rcp
SRCS=	rcp.c util.c
#SRCS+=	krcmd.c kcmd.c
#CFLAGS+=-DKERBEROS -DCRYPT
#DPADD=	${LIBKRB} ${LIBDES}
#LDADD=	-lkrb -ldes
BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg
.PATH:	${.CURDIR}/../../usr.bin/rlogin

.include <bsd.prog.mk>
