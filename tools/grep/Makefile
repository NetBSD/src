#	$NetBSD: Makefile,v 1.2.42.1 2018/09/06 06:56:49 pgoyette Exp $
#

HOSTPROGNAME=   ${_TOOL_PREFIX}grep
HOST_SRCDIR=    usr.bin/grep

HOST_CPPFLAGS+=	-DWITHOUT_NLS -DWITHOUT_BZ2 -DWITHOUT_GZIP

.include "${.CURDIR}/../Makefile.host"
