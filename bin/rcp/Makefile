#	$NetBSD: Makefile,v 1.22 2003/07/23 08:01:24 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=		rcp
SRCS=		rcp.c util.c

# XXX Kerberos support broken right now.
USE_KERBEROS4=	no

.if (${USE_KERBEROS4} != "no")
RLOGIN= 	${NETBSDSRCDIR}/usr.bin/rlogin
.PATH:		${RLOGIN}

SRCS+=		krcmd.c kcmd.c
CPPFLAGS+=	-DKERBEROS -DCRYPT -I${RLOGIN}
LDADD+= 	-lkrb -ldes
DPADD+= 	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
