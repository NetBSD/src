#	$NetBSD: Makefile,v 1.5 2001/08/06 19:12:05 tv Exp $

PROG=		objdump

.include <bsd.own.mk>
.if defined(USE_NEW_TOOLCHAIN)
.include "${.CURDIR}/../common/Makefile.prog"
.else
SRCS=		objdump.c prdbg.c
MAN=		objdump.1

.include <bsd.prog.mk>
.endif
