#	$NetBSD: Makefile.inc,v 1.3 2002/10/17 11:53:32 scw Exp $

SRCS+=	__assert.c __main.c \
	imax.S imin.S \
	lmax.S lmin.S \
	max.S memchr.c memcmp.c memcpy.S memmove.c memset.S min.S \
	scanc.S skpc.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S \
	strncmp.S strncpy.S strrchr.S strtoul.c \
	strcasecmp.S strncasecmp.S \
	random.S \
	ulmax.S ulmin.S \
	sdivsi3.S udivsi3.S

