#	$NetBSD: Makefile.inc,v 1.2 2002/12/08 09:48:36 scw Exp $

SRCS+=	bzero.S \
	index.S \
	memcpy.S memset.S \
	rindex.S \
	strcasecmp.S strcat.S strchr.S strcmp.S \
	strcpy.S strlen.S strncasecmp.S strncat.S \
	strncmp.S strncpy.S strrchr.S swab.S

SRCS+=	bcmp.c bcopy.c \
	ffs.c \
	memchr.c memcmp.c \
	strcspn.c strpbrk.c strsep.c strspn.c \
	strstr.c

