#	$Id: Makefile,v 1.4 1999/02/13 02:54:23 lukem Exp $

CPPFLAGS+= -I${.CURDIR}

LIB=	gnumalloc
SRCS+=	combined.c
#SRCS+=	malloc.c free.c realloc.c
SRCS+=	cfree.c calloc.c morecore.c
SRCS+=	memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=	ralloc.c
MKMAN=	no

.include <bsd.lib.mk>
