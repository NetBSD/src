#	$NetBSD: Makefile,v 1.13 2025/09/18 03:36:02 mrg Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client

.PATH.c: ${.CURDIR}/../ypcat
CPPFLAGS+= -I${.CURDIR}/../ypcat
PROG=	ypmatch
SRCS=	ypmatch.c ypalias_init.c

COPTS.ypalias_init.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.prog.mk>
