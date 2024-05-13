# $NetBSD: Makefile,v 1.4 2024/05/13 20:38:05 rillig Exp $

SRCS=		flashctl.c

PROG=		flashctl
MAN=		flashctl.8
LINTFLAGS+=	${MKLLVM:U-T:D}	# strict bool mode

WARNS=		4

.include <bsd.prog.mk>

