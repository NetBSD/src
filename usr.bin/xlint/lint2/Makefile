#	$NetBSD: Makefile,v 1.9 2001/10/24 02:31:10 thorpej Exp $

.PATH:	${.CURDIR}/../lint1

.if !defined(XLINT_TARGET_PREFIX)
XLINT_TARGET_PREFIX=
.endif

PROG=	lint2
PROGNAME=${XLINT_TARGET_PREFIX}${PROG}
SRCS=	main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c
MKMAN=	no
CPPFLAGS+=-I${.CURDIR}/../lint1
LINTFLAGS=-abehrz
WFORMAT=1		# hopeless
BINDIR=	/usr/libexec

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

