#	$NetBSD: Makefile.inc,v 1.3 2003/08/01 17:03:51 lukem Exp $

# objects built from assembler sources (need lint stubs)
# hton* and nto* functions provided by ../gen/byte_swap_*.S
SRCS+=  

SRCS.mips.net=	Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
LSRCS+=		${SRCS.mips.net}
DPSRCS+=	${SRCS.mips.net}
CLEANFILES+=	${SRCS.mips.net}
