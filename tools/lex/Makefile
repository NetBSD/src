#	$NetBSD: Makefile,v 1.6 2009/10/26 04:08:55 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}lex
HOST_SRCDIR=	usr.bin/lex
#HOST_SRCDIR=	external/bsd/flex/bin

.include "${.CURDIR}/../Makefile.host"

scan.c:
	echo '#include <initscan.c>' >$@
