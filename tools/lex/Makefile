#	$NetBSD: Makefile,v 1.5 2009/10/26 01:19:02 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}lex
HOST_SRCDIR=	external/bsd/flex/bin

.include "${.CURDIR}/../Makefile.host"

scan.c:
	echo '#include <initscan.c>' >$@
