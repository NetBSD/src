#	$NetBSD: Makefile,v 1.8 1997/05/08 21:11:03 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rcp
SRCS=	rcp.c util.c
BINOWN=	root
BINMODE=4555
#INSTALLFLAGS=-fschg

.if defined(KERBEROS)
.PATH:	${.CURDIR}/../../usr.bin/rlogin
SRCS+=	krcmd.c kcmd.c
CFLAGS+=-DKERBEROS -DCRYPT
LDADD+=	-lkrb -ldes
DPADD+=	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
