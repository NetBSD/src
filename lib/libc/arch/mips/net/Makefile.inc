#	$NetBSD: Makefile.inc,v 1.2 1999/01/15 13:31:19 bouyer Exp $

# objects built from assembler sources (need lint stubs)
# hton* and nto* functions provided by ../gen/byte_swap_*.S
SRCS+=  

LSRCS+= Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
DPSRCS+= Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
