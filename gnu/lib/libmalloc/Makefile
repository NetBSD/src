#	$Id: Makefile,v 1.5 2000/07/03 05:05:25 matt Exp $

CPPFLAGS+= -I${.CURDIR} -DSTDC_HEADERS -DHAVE_STDLIB_H

LIB=	gnumalloc
SRCS+=	combined.c
#SRCS+=	malloc.c free.c realloc.c
SRCS+=	cfree.c calloc.c morecore.c
SRCS+=	memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=	ralloc.c
MKMAN=	no

.include <bsd.lib.mk>
