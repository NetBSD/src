#	$Id: Makefile,v 1.3 1997/10/25 09:27:35 lukem Exp $

CPPFLAGS+= -I${.CURDIR}

LIB=	gnumalloc
SRCS+=	combined.c
#SRCS+=	malloc.c free.c realloc.c
SRCS+=	cfree.c calloc.c morecore.c
SRCS+=	memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=	ralloc.c
NOMAN=	noman

.include <bsd.lib.mk>
