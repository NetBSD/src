#	$NetBSD: Makefile,v 1.1 2002/06/18 08:21:59 itojun Exp $

PROG=		coffdump

.include <bsd.own.mk>
.if defined(USE_NEW_TOOLCHAIN)
.include "${.CURDIR}/../common/Makefile.prog"
.else
SRCS=		coffdump.c coffgrok.c
NOMAN=		yes

.include <bsd.prog.mk>
.endif
