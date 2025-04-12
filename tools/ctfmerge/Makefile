#	$NetBSD: Makefile,v 1.13 2025/04/12 10:00:17 rin Exp $

.include <bsd.hostinit.mk>

.include "${.CURDIR}/../Makefile.ctftools"

HOSTPROGNAME=   ${_TOOL_PREFIX}ctfmerge
HOST_SRCDIR=    external/cddl/osnet/usr.bin/ctfmerge

# need native pthread support
HOST_CFLAGS+=	-pthread

.include "${.CURDIR}/../Makefile.host"
