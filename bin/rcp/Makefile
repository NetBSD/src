#	$NetBSD: Makefile,v 1.19 2001/12/12 00:07:45 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

# XXX Kerberos support broken right now.
NOKERBEROS=	# defined

.include <bsd.own.mk>

PROG=		rcp
SRCS=		rcp.c util.c

.if (${MKKERBEROS} != "no")
RLOGIN= 	${.CURDIR}/../../usr.bin/rlogin
.PATH:		${RLOGIN}

SRCS+=		krcmd.c kcmd.c
CPPFLAGS+=	-DKERBEROS -DCRYPT -I${RLOGIN}
LDADD+= 	-lkrb -ldes
DPADD+= 	${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
