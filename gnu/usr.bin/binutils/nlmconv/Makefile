#	$NetBSD: Makefile,v 1.4 2002/06/18 08:21:59 itojun Exp $

PROG=		nlmconv

.include <bsd.own.mk>
.if defined(USE_NEW_TOOLCHAIN)
.include "${.CURDIR}/../common/Makefile.prog"
.else
SRCS=		nlmconv.c nlmheader.y 
MAN=		nlmconv.1

.include <bsd.prog.mk>
.endif
