#	$NetBSD: Makefile,v 1.3 2000/11/08 06:02:01 itojun Exp $
#	$KAME: Makefile,v 1.5 2000/11/08 05:58:24 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

PROG=	bftest
SRCS=	bftest.c bf_enc.c bf_skey.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR}

.PATH:	${SYSDIR}/crypto/blowfish

.include <bsd.prog.mk>
