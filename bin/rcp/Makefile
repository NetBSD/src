#	$NetBSD: Makefile,v 1.13 1999/02/17 06:47:51 scottr Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rcp
SRCS=	rcp.c util.c

.if defined(KERBEROS)
RLOGIN=	${.CURDIR}/../../usr.bin/rlogin
.PATH:	${RLOGIN}
SRCS+=	krcmd.c kcmd.c
CPPFLAGS+=-DKERBEROS -DCRYPT -I ${RLOGIN}
LDADD+=	-lkrb -ldes
DPADD+=	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
