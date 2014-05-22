#	$NetBSD: Makefile.inc,v 1.4.6.1 2014/05/22 11:36:50 yamt Exp $

# objects built from assembler sources
SRCS+=	bcmp.S bcopy.S ffs.S memchr.S memcmp.S \
	memcpy.S memmove.S memset.S strcat.S strchr.S \
	strcmp.S strcpy.S strlen.S strncmp.S strrchr.S swab.S
NO_SRCS+= bzero.c
