#	$NetBSD: Makefile,v 1.9 1997/05/27 08:45:52 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

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
