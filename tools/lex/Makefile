#	$NetBSD: Makefile,v 1.7 2009/10/26 21:32:14 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}lex
HOST_SRCDIR=	external/bsd/flex/bin

.include "${.CURDIR}/../Makefile.host"
HOST_CPPFLAGS+=	-DM4=\"${TOOL_M4}\"

scan.c:
	echo '#include <initscan.c>' >$@
