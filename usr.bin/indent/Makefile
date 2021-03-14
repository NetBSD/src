#	$NetBSD: Makefile,v 1.11 2021/03/14 00:22:16 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	indent
SRCS=	indent.c io.c lexi.c parse.c pr_comment.c args.c

CPPFLAGS+=	${DEBUG:D-Ddebug}
LINTFLAGS+=	-e -w

# bug in lint; see tests/usr.bin/lint/lint1/msg_168.c
LINTFLAGS.lexi.c+=	-X 168

.include <bsd.prog.mk>
