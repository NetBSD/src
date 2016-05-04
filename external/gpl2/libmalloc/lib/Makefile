#	$NetBSD: Makefile,v 1.5 2016/05/04 17:17:00 christos Exp $

NOMAN=	# defined
.include <bsd.init.mk>

LIB=	gnumalloc
SRCS+=  combined.c
#SRCS+= malloc.c free.c realloc.c
SRCS+=  cfree.c calloc.c morecore.c
SRCS+=  memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=  ralloc.c


CPPFLAGS+= -I${DIST} -I${.CURDIR} -DSTDC_HEADERS -DHAVE_STDLIB_H -DHAVE_UNISTD_H

.if ${HAVE_GCC:U0} >= 53
COPTS+=-fno-builtin-malloc
.endif

.PATH: ${DIST}

.include <bsd.lib.mk>
