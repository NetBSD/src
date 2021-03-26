#	$NetBSD: Makefile,v 1.12 2021/03/26 22:27:43 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	indent.c io.c lexi.c parse.c pr_comment.c args.c

CPPFLAGS+=	${DEBUG:D-Ddebug}
LINTFLAGS+=	-e -w

.include <bsd.prog.mk>
