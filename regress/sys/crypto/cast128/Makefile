#	$NetBSD: Makefile,v 1.3 2000/11/08 06:02:01 itojun Exp $
#	$KAME: Makefile,v 1.4 2000/11/08 05:58:25 itojun Exp $

.include "../Makefile.inc"
.include <bsd.own.mk>

PROG=	casttest
SRCS=	casttest.c cast128.c
MKMAN=	no

CPPFLAGS+=	-I${SYSDIR}

.PATH:	${SYSDIR}/crypto/cast128

.include <bsd.prog.mk>
