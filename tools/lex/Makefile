#	$NetBSD: Makefile,v 1.4 2002/12/08 20:20:01 thorpej Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}lex
HOST_SRCDIR=	usr.bin/lex

.include "${.CURDIR}/../Makefile.host"

scan.c:
	echo '#include <initscan.c>' >$@
