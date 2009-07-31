#	$NetBSD: Makefile.inc,v 1.7 2009/07/31 20:39:59 dsl Exp $

# objects built from assembler sources
SRCS+=	abs.S div.S labs.S ldiv.S llabs.S
NO_SRCS+= imaxabs.c imaxdiv.c
