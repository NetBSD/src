#	$NetBSD: Makefile,v 1.10 1997/07/20 20:47:31 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

WARNS=	1
PROG=	rcp
SRCS=	rcp.c util.c

.if defined(KERBEROS)
.PATH:	${.CURDIR}/../../usr.bin/rlogin
SRCS+=	krcmd.c kcmd.c
CFLAGS+=-DKERBEROS -DCRYPT
LDADD+=	-lkrb -ldes
DPADD+=	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
