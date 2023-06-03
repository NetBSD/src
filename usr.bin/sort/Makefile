#	$NetBSD: Makefile,v 1.10 2023/06/03 09:09:18 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	sort
SRCS=	append.c fields.c files.c fsort.c init.c msort.c sort.c tmp.c
SRCS+=	radix_sort.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

COPTS.init.c+=		${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
