#	$NetBSD: Makefile.inc,v 1.1 2002/07/05 13:31:31 scw Exp $

# hton* and nto* functions provided by ../gen/byte_swap_*.S
SRCS+=

LSRCS+= Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
DPSRCS+= Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
