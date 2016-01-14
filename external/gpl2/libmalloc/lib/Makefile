#	$NetBSD: Makefile,v 1.2 2016/01/14 02:43:50 christos Exp $

NOMAN=	# defined
.include <bsd.init.mk>

LIB=	gnumalloc
SRCS+=  combined.c
#SRCS+= malloc.c free.c realloc.c
SRCS+=  cfree.c calloc.c morecore.c
SRCS+=  memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=  ralloc.c


CPPFLAGS+= -I${DIST} -I${.CURDIR} -DSTDC_HEADERS -DHAVE_STDLIB_H

.PATH: ${DIST}

.include <bsd.lib.mk>
