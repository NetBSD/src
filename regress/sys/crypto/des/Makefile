#	$NetBSD: Makefile,v 1.3 2000/11/08 06:02:02 itojun Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:25 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

PROG=	destest
SRCS=	destest.c des_ecb.c des_setkey.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR}

.PATH:	${SYSDIR}/crypto/des

.include <bsd.prog.mk>
