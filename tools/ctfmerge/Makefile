#	$NetBSD: Makefile,v 1.12.2.1 2025/08/02 05:58:22 perseant Exp $

.include <bsd.hostinit.mk>

.include "${.CURDIR}/../Makefile.ctftools"

HOSTPROGNAME=   ${_TOOL_PREFIX}ctfmerge
HOST_SRCDIR=    external/cddl/osnet/usr.bin/ctfmerge

# need native pthread support
HOST_CFLAGS+=	-pthread

.include "${.CURDIR}/../Makefile.host"
