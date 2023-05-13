#	$NetBSD: Makefile,v 1.15 2023/05/13 09:27:49 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	args.c debug.c indent.c io.c lexi.c parse.c pr_comment.c

CPPFLAGS+=	${DEBUG:D-Ddebug}
LINTFLAGS+=	-e -w -T
WARNS=		6

.include <bsd.prog.mk>
