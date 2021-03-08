#	$NetBSD: Makefile,v 1.9 2021/03/08 19:21:41 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	indent.c io.c lexi.c parse.c pr_comment.c args.c

COPTS.io.c += -Wno-error=format-nonliteral
CPPFLAGS+=	${DEBUG:D-Ddebug}

LINTFLAGS+=	-e

.include <bsd.prog.mk>
