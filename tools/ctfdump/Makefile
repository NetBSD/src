#	$NetBSD: Makefile,v 1.1.2.2 2025/04/14 09:58:18 martin Exp $

.include <bsd.hostinit.mk>

.include "${.CURDIR}/../Makefile.ctftools"

HOSTPROGNAME=   ${_TOOL_PREFIX}ctfdump
HOST_SRCDIR=    external/cddl/osnet/usr.bin/ctfdump
#HOST_SRCS=	dump.c \
		symbol.c \
		utils.c

# need native pthread support
HOST_CFLAGS+=	-pthread

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
