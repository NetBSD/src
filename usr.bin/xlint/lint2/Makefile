#	$NetBSD: Makefile,v 1.20 2021/04/18 22:51:24 rillig Exp $

NOMAN=		# defined

PROG=		lint2
SRCS=		main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c \
		inittyp.c tyname.c
BINDIR=		/usr/libexec
CPPFLAGS+=	-I${.CURDIR}

COPTS.msg.c+=	${${ACTIVE_CC} == "clang":? -Wno-format-nonliteral :}

.include <bsd.prog.mk>
