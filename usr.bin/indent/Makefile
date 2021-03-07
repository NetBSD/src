#	$NetBSD: Makefile,v 1.8 2021/03/07 10:56:18 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	indent.c io.c lexi.c parse.c pr_comment.c args.c

COPTS.io.c += -Wno-error=format-nonliteral

LINTFLAGS+=	-e

.include <bsd.prog.mk>
