#	from: @(#)Makefile	5.1 (Berkeley) 3/8/91
#	$NetBSD: Makefile,v 1.4 2006/08/26 18:15:37 christos Exp $

PROG=	netgroup_mkdb
SRCS=	netgroup_mkdb.c str.c
MAN=	netgroup_mkdb.8

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
