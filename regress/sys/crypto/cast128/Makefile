#	$NetBSD: Makefile,v 1.5 2002/09/18 05:41:51 lukem Exp $
#	$KAME: Makefile,v 1.4 2000/11/08 05:58:25 itojun Exp $

NOMAN=		# defined

PROG=		casttest
SRCS=		casttest.c cast128.c

.include "../Makefile.inc"
.include <bsd.own.mk>

CPPFLAGS+=	-I${SYSDIR}
.PATH:		${SYSDIR}/crypto/cast128

.include <bsd.prog.mk>
