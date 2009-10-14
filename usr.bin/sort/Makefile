#	$NetBSD: Makefile,v 1.5.40.1 2009/10/14 20:41:53 sborrill Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	sort
SRCS=	append.c fields.c files.c fsort.c init.c msort.c sort.c tmp.c
SRCS+=	radix_sort.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
