#	$NetBSD: Makefile,v 1.13 2025/09/18 03:36:02 mrg Exp $
#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90

USE_FORT?= yes	# network client

PROG=		ypcat
SRCS=		ypalias_init.c ypcat.c
FILES=		nicknames
FILESDIR=	/var/yp
MAN=		ypcat.1 nicknames.5

COPTS.ypalias_init.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.prog.mk>
