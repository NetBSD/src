#	$NetBSD: Makefile.inc,v 1.6.26.1 2020/04/08 14:07:12 martin Exp $

# objects built from assembler sources
SRCS+=	bcopy.S ffs.S memchr.S  \
	memcpy.S memmove.S memset.S strcat.S strchr.S \
	strcmp.S strcpy.S strlen.S strncmp.S strrchr.S swab.S
NO_SRCS+= bzero.c
