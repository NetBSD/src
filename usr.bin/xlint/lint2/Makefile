#	$NetBSD: Makefile,v 1.6.2.1 2000/10/19 16:33:22 he Exp $

.PATH:	${.CURDIR}/../lint1

PROG=	lint2
SRCS=	main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c
MKMAN=	no
CPPFLAGS+=-I${.CURDIR}/../lint1
LINTFLAGS=-abehrz
WFORMAT=1		# hopeless
BINDIR=	/usr/libexec

.include <bsd.prog.mk>
