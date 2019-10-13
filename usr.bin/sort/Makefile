#	$NetBSD: Makefile,v 1.9 2019/10/13 07:28:17 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	sort
SRCS=	append.c fields.c files.c fsort.c init.c msort.c sort.c tmp.c
SRCS+=	radix_sort.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

COPTS.init.c+=		${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
