#	$NetBSD: Makefile,v 1.8 2004/06/30 03:26:27 jmc Exp $

NOMAN=	# defined

LIB=	gnumalloc
SRCS+=	combined.c
#SRCS+=	malloc.c free.c realloc.c
SRCS+=	cfree.c calloc.c morecore.c
SRCS+=	memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=	ralloc.c

CPPFLAGS+= -I${.CURDIR} -DSTDC_HEADERS -DHAVE_STDLIB_H

.include <bsd.lib.mk>
