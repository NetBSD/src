#	$NetBSD: Makefile,v 1.1 2025/04/12 10:07:53 rin Exp $

.include <bsd.hostinit.mk>

.include "${.CURDIR}/../Makefile.ctftools"

HOSTPROGNAME=	${_TOOL_PREFIX}ctfdump
HOST_SRCDIR=	external/cddl/osnet/usr.bin/ctfdump
#HOST_SRCS=	dump.c \
		symbol.c \
		utils.c

# need native pthread support
HOST_CFLAGS+=	-pthread

.include "${.CURDIR}/../Makefile.host"
