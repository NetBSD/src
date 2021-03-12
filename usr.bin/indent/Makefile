#	$NetBSD: Makefile,v 1.10 2021/03/12 19:11:29 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	indent.c io.c lexi.c parse.c pr_comment.c args.c

CPPFLAGS+=	${DEBUG:D-Ddebug}
LINTFLAGS+=	-e

.include <bsd.prog.mk>
