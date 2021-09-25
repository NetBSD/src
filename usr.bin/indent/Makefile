#	$NetBSD: Makefile,v 1.13 2021/09/25 17:11:23 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	indent.c io.c lexi.c parse.c pr_comment.c args.c

CPPFLAGS+=	${DEBUG:D-Ddebug}
LINTFLAGS+=	-e -w -T

.include <bsd.prog.mk>
