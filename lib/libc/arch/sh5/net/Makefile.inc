#	$NetBSD: Makefile.inc,v 1.2 2003/08/01 17:03:55 lukem Exp $

# hton* and nto* functions provided by ../gen/byte_swap_*.S
SRCS+=

SRCS.sh5.net=	Lint_htonl.c Lint_htons.c Lint_ntohl.c Lint_ntohs.c
LSRCS+=		${SRCS.sh5.net}
DPSRCS+=	${SRCS.sh5.net}
CLEANFILES+=	${SRCS.sh5.net}
