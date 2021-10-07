#	$NetBSD: Makefile,v 1.14 2021/10/07 18:32:09 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	indent.c io.c lexi.c parse.c pr_comment.c args.c

CPPFLAGS+=	${DEBUG:D-Ddebug}
LINTFLAGS+=	-e -w -T
WARNS=		6

.include <bsd.prog.mk>
