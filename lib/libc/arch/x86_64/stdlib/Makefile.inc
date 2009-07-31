#	$NetBSD: Makefile.inc,v 1.5 2009/07/31 20:40:00 dsl Exp $

# objects built from assembler sources
SRCS+=	abs.S div.S labs.S ldiv.S
NO_SRCS+= llabs.c imaxabs.c imaxdiv.c
