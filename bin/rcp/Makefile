#	$NetBSD: Makefile,v 1.20 2002/03/22 18:10:19 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=		rcp
SRCS=		rcp.c util.c

# XXX Kerberos support broken right now.
USE_KERBEROS=	no

.if (${USE_KERBEROS} != "no")
RLOGIN= 	${.CURDIR}/../../usr.bin/rlogin
.PATH:		${RLOGIN}

SRCS+=		krcmd.c kcmd.c
CPPFLAGS+=	-DKERBEROS -DCRYPT -I${RLOGIN}
LDADD+= 	-lkrb -ldes
DPADD+= 	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
