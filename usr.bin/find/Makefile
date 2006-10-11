#	$NetBSD: Makefile,v 1.11 2006/10/11 19:51:10 apb Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	find
SRCS=	find.c function.c ls.c main.c misc.c operator.c option.c stat_flags.c
WARNS=	4

CPPFLAGS+=	-I${NETBSDSRCDIR}/bin/ls
.PATH:	${NETBSDSRCDIR}/bin/ls

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
