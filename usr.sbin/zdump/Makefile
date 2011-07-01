#	$NetBSD: Makefile,v 1.6 2011/07/01 02:42:10 mrg Exp $

.include <bsd.own.mk>

PROG=	zdump
SRCS=	zdump.c ialloc.c
MAN=	zdump.8

.PATH: ${NETBSDSRCDIR}/lib/libc/time
.include <bsd.prog.mk>

# XXX
.if defined(HAVE_GCC) && ${HAVE_GCC} >= 45
COPTS.zdump.c+=	-Wno-strict-overflow
.endif
