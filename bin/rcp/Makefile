#	$NetBSD: Makefile,v 1.18 2000/06/23 06:01:12 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=	rcp
SRCS=	rcp.c util.c

# XXX Kerberos support broken right now.
MKKERBEROS=no

.if (${MKKERBEROS} != "no")

RLOGIN= ${.CURDIR}/../../usr.bin/rlogin
SRCS+=	krcmd.c kcmd.c
CPPFLAGS+= -DKERBEROS -DCRYPT -I${RLOGIN}

LDADD+= -lkrb -ldes
DPADD+= ${LIBKRB} ${LIBDES}

.PATH:	${RLOGIN}
.endif

.include <bsd.prog.mk>
