#	$NetBSD: Makefile,v 1.10 2006/08/26 18:17:42 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	find
SRCS=	find.c function.c ls.c main.c misc.c operator.c option.c stat_flags.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/bin/ls
.PATH:	${NETBSDSRCDIR}/bin/ls

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
