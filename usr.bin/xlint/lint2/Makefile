#	$NetBSD: Makefile,v 1.18 2021/01/16 00:09:28 rillig Exp $

NOMAN=		# defined

PROG=		lint2
SRCS=		main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c \
		inittyp.c tyname.c
BINDIR=		/usr/libexec
CPPFLAGS+=	-DPASS=\"${PROG}.h\" -I${.CURDIR}

.include <bsd.prog.mk>
