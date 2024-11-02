#	$NetBSD: Makefile.inc,v 1.6 2024/11/02 18:39:21 jakllsch Exp $

SRCS+=	bcmp.S bcopy.S bzero.S \
	ffs.S \
	memcpy.S memmove.S \
	strchr.S strrchr.S \
	strcmp.S strlen.S
NO_SRCS+=	ffsl.c
