#	$NetBSD: Makefile.inc,v 1.4 2003/08/01 17:03:46 lukem Exp $

# objects built from assembler sources (need lint stubs)
# hton* and nto* functions provided by ../gen/byte_swap_*.S
SRCS+=  

SRCS.alpha.net=	Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
LSRCS+=		${SRCS.alpha.net}
DPSRCS+=	${SRCS.alpha.net}
CLEANFILES+=	${SRCS.alpha.net}
