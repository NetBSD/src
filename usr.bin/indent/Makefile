#	$NetBSD: Makefile,v 1.7 2019/04/04 15:22:13 kamil Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	indent.c io.c lexi.c parse.c pr_comment.c args.c

COPTS.io.c += -Wno-error=format-nonliteral

.include <bsd.prog.mk>
