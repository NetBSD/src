#	$NetBSD: Makefile.inc,v 1.2 2003/08/01 17:03:57 lukem Exp $

# objects built from assembler sources (need lint stubs)
# hton* and nto* functions provided by ../gen/byte_swap_*.S
SRCS+=  

SRCS.x86_64.net=Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
LSRCS+=		${SRCS.x86_64.net}
DPSRCS+=	${SRCS.x86_64.net}
CLEANFILES+=	${SRCS.x86_64.net}
