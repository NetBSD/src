#	$NetBSD: Makefile.inc,v 1.1.10.1 1997/11/08 21:52:25 veego Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=  htonl.S htons.S ntohl.S ntohs.S

LSRCS+=	Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
DPSRCS+= Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
