#	$NetBSD: Makefile,v 1.8 2009/10/29 17:16:40 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}lex
HOST_SRCDIR=	external/bsd/flex/bin

.include "${.CURDIR}/../Makefile.host"
HOST_CPPFLAGS+=	-DM4=\"${TOOL_M4}\"

scan.c:
	echo '#include <initscan.c>' >$@
parse.c:
	echo '#include <initparse.c>' >$@
parse.h:
	echo '#include <initparse.h>' >$@

scan.c: parse.h

CLEANFILES+=scan.c parse.c parse.h
