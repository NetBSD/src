#	$NetBSD: Makefile,v 1.6.4.1 1997/02/17 15:18:41 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rcp
SRCS=	rcp.c util.c
#INSTALLFLAGS=-fschg

.if defined(KERBEROS)
.PATH:	${.CURDIR}/../../usr.bin/rlogin
SRCS+=	krcmd.c kcmd.c
CFLAGS+=-DKERBEROS -DCRYPT
LDADD+=	-lkrb -ldes
DPADD+=	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
