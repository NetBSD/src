#	$NetBSD: Makefile,v 1.12 1997/10/10 19:56:54 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rcp
SRCS=	rcp.c util.c

.if defined(KERBEROS)
.PATH:	${.CURDIR}/../../usr.bin/rlogin
SRCS+=	krcmd.c kcmd.c
CPPFLAGS+=-DKERBEROS -DCRYPT
LDADD+=	-lkrb -ldes
DPADD+=	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
