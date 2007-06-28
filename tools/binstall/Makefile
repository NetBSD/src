#	$NetBSD: Makefile,v 1.6 2007/06/28 19:05:55 joerg Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}install
HOST_SRCDIR=	usr.bin/xinstall
CPPFLAGS+=	-I${.CURDIR}/../compat/sys
CPPFLAGS+=	-DTARGET_STRIP=\"${STRIP}\"

.include "${.CURDIR}/../Makefile.host"

INSTALL=	./xinstall
