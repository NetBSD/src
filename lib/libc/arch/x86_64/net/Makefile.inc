#	$NetBSD: Makefile.inc,v 1.1 2001/06/19 00:25:03 fvdl Exp $

# objects built from assembler sources (need lint stubs)
# hton* and nto* functions provided by ../gen/byte_swap_*.S
SRCS+=  

LSRCS+=	Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
DPSRCS+= Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
