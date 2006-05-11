#	$NetBSD: Makefile,v 1.3 2006/05/11 08:42:05 mrg Exp $

PROG=	wiconfig
MAN=	wiconfig.8

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-fno-strict-aliasing
.endif
