#	$NetBSD: Makefile.inc,v 1.5 2006/03/22 22:03:58 drochner Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=abs.S labs.S

# objects built from C sources
SRCS+=	div.c ldiv.c llabs.c

SRCS+=	erand48_ieee754.c
