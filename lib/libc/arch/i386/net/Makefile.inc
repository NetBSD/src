#	$NetBSD: Makefile.inc,v 1.4 2003/08/01 17:03:48 lukem Exp $

# objects built from assembler sources (need lint stubs)
# hton* and nto* functions provided by ../gen/byte_swap_*.S
SRCS+=  

SRCS.i386.net=	Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
LSRCS+=		${SRCS.i386.net}
DPSRCS+=	${SRCS.i386.net}
CLEANFILES+=	${SRCS.i386.net}
