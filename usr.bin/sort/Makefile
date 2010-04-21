#	$NetBSD: Makefile,v 1.5.46.1 2010/04/21 05:27:12 matt Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	sort
SRCS=	append.c fields.c files.c fsort.c init.c msort.c sort.c tmp.c
SRCS+=	radix_sort.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
