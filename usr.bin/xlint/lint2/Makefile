#	$NetBSD: Makefile,v 1.11 2001/12/12 01:48:48 tv Exp $

.PATH:		${.CURDIR}/../lint1

PROG=		lint2
SRCS=		main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c
NOMAN=		# defined
CPPFLAGS+=	-I${.CURDIR}/../lint1
LINTFLAGS=	-abehrz
WFORMAT=1		# hopeless
BINDIR=		/usr/libexec

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif

