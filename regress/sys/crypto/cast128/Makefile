#	$NetBSD: Makefile,v 1.4 2001/12/12 01:24:14 tv Exp $
#	$KAME: Makefile,v 1.4 2000/11/08 05:58:25 itojun Exp $

PROG=		casttest
SRCS=		casttest.c cast128.c
NOMAN=		# defined

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR}
.PATH:		${SYSDIR}/crypto/cast128

.include <bsd.prog.mk>
