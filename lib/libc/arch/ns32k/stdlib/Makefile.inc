#	$NetBSD: Makefile.inc,v 1.4 2000/03/06 18:32:25 kleink Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=abs.S labs.S

# objects built from C sources
SRCS+=	div.c ldiv.c llabs.c
