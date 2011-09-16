#	$NetBSD: Makefile.inc,v 1.2 2011/09/16 16:41:20 joerg Exp $

.PATH: ${.PARSEDIR}/../dist
WARNS?=		4
CWARNFLAGS+=	-Wno-missing-noreturn
