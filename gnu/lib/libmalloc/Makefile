#	$Id: Makefile,v 1.2 1993/10/26 06:52:16 cgd Exp $

CFLAGS+= -I${.CURDIR}

LIB=	gnumalloc
SRCS+=	combined.c
#SRCS+=	malloc.c free.c realloc.c
SRCS+=	cfree.c calloc.c morecore.c
SRCS+=	memalign.c valloc.c mcheck.c mtrace.c mstats.c vm-limit.c
SRCS+=	ralloc.c
NOMAN=	noman

.include <bsd.lib.mk>
