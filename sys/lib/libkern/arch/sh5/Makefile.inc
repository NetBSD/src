#	$NetBSD: Makefile.inc,v 1.1.4.3 2002/10/10 18:43:26 jdolecek Exp $

SRCS+=	__assert.c __main.c \
	ffs.c \
	imax.c imin.c \
	lmax.c lmin.c \
	max.c memchr.c memcmp.c memcpy.c memmove.c memset.c min.c \
	scanc.c skpc.c \
	strcat.c strcmp.c strcpy.c strlen.c strcasecmp.c strncasecmp.c \
	strncmp.c strncpy.c strtoul.c \
	random.c \
	ulmax.c ulmin.c \
	sdivsi3.S udivsi3.S

