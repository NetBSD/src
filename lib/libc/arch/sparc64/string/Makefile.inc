#	$NetBSD: Makefile.inc,v 1.7 2009/07/30 21:26:36 dsl Exp $

SRCS+=	ffs.S memcpy.S memset.S strlen.S
NO_SRCS+= bcopy.c bzero.c
