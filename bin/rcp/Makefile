#	$NetBSD: Makefile,v 1.21 2002/08/19 09:56:02 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=		rcp
SRCS=		rcp.c util.c

# XXX Kerberos support broken right now.
USE_KERBEROS=	no

.if (${USE_KERBEROS} != "no")
RLOGIN= 	${NETBSDSRCDIR}/usr.bin/rlogin
.PATH:		${RLOGIN}

SRCS+=		krcmd.c kcmd.c
CPPFLAGS+=	-DKERBEROS -DCRYPT -I${RLOGIN}
LDADD+= 	-lkrb -ldes
DPADD+= 	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
