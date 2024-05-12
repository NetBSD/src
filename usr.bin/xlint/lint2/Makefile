#	$NetBSD: Makefile,v 1.24 2024/05/12 19:00:10 rillig Exp $

NOMAN=		# defined

PROG=		lint2
SRCS=		main2.c hash.c read.c mem.c chk.c msg.c emit.c emit2.c \
		inittyp.c tyname.c
BINDIR=		/usr/libexec
CPPFLAGS+=	-I${.CURDIR}
CPPFLAGS+=	-DIS_LINT2

COPTS.msg.c+=	${${ACTIVE_CC} == "clang":? -Wno-format-nonliteral :}

.include <bsd.prog.mk>
