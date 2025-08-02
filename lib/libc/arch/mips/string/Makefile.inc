#	$NetBSD: Makefile.inc,v 1.5.58.1 2025/08/02 05:54:33 perseant Exp $

SRCS+=	bcmp.S bcopy.S bzero.S \
	ffs.S \
	memcpy.S memmove.S \
	strchr.S strrchr.S \
	strcmp.S strlen.S
NO_SRCS+=	ffsl.c
