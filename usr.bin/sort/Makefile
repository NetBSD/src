#	$NetBSD: Makefile,v 1.11 2023/08/03 13:36:09 rin Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	sort
SRCS=	append.c fields.c files.c fsort.c init.c msort.c sort.c tmp.c
SRCS+=	radix_sort.c

LDADD+=-lutil
DPADD+=${LIBUTIL}

COPTS.files.c+=		${CC_WNO_USE_AFTER_FREE}
COPTS.init.c+=		${CC_WNO_FORMAT_TRUNCATION}
COPTS.sort.c+=		${CC_WNO_USE_AFTER_FREE}

.include <bsd.prog.mk>
