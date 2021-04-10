#	$NetBSD: Makefile,v 1.19 2021/04/10 18:36:27 rillig Exp $

NOMAN=		# defined

PROG=		lint2
SRCS=		main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c \
		inittyp.c tyname.c
BINDIR=		/usr/libexec
CPPFLAGS+=	-I${.CURDIR}

.include <bsd.prog.mk>
