#	$NetBSD: Makefile,v 1.3 2016/01/15 15:14:02 christos Exp $

NOMAN=	# defined
.include <bsd.init.mk>

LIB=	gnumalloc
SRCS+=  combined.c
#SRCS+= malloc.c free.c realloc.c
SRCS+=  cfree.c calloc.c morecore.c
SRCS+=  memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=  ralloc.c


CPPFLAGS+= -I${DIST} -I${.CURDIR} -DSTDC_HEADERS -DHAVE_STDLIB_H -DHAVE_UNISTD_H

.PATH: ${DIST}

.include <bsd.lib.mk>
