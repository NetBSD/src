#	$NetBSD: Makefile.inc,v 1.3 2000/02/06 13:14:51 kleink Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=abs.S labs.S

# objects built from C sources
SRCS+=	div.c ldiv.c
