#	$NetBSD: Makefile,v 1.8 1999/01/03 14:54:27 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	find
SRCS=	find.c function.c ls.c main.c misc.c operator.c option.c stat_flags.c

CPPFLAGS+=	-I${.CURDIR}/../../bin/ls
.PATH:	${.CURDIR}/../../bin/ls

.include <bsd.prog.mk>
