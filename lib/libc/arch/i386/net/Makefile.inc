#	$NetBSD: Makefile.inc,v 1.2 1997/11/05 23:43:00 cgd Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=  htonl.S htons.S ntohl.S ntohs.S

LSRCS+=	Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
DPSRCS+= Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
