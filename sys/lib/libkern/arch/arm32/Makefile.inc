#	$NetBSD: Makefile.inc,v 1.7 1997/10/09 15:42:35 bouyer Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.c bzero.c ffs.c strcat.c strcmp.c strcpy.c strlen.c strncmp.c \
	strncpy.c scanc.c skpc.c htonl.c htons.c ntohl.c ntohs.c \
	div.S random.c strncasecmp.c __assert.c strchr.c
