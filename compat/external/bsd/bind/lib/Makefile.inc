#	$NetBSD: Makefile.inc,v 1.1 2009/07/21 01:00:37 mrg Exp $

.include <bsd.own.mk>

_CURDIR:= ${.CURDIR}

.include "${NETBSDSRCDIR}/external/bsd/bind/lib/Makefile.inc"

.CURDIR:= ${_CURDIR}
