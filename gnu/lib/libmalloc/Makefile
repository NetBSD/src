#	$Id: Makefile,v 1.6 2001/12/12 01:48:58 tv Exp $

CPPFLAGS+= -I${.CURDIR} -DSTDC_HEADERS -DHAVE_STDLIB_H

LIB=	gnumalloc
SRCS+=	combined.c
#SRCS+=	malloc.c free.c realloc.c
SRCS+=	cfree.c calloc.c morecore.c
SRCS+=	memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=	ralloc.c
NOMAN=	# defined

.include <bsd.lib.mk>
