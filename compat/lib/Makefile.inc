#	$NetBSD: Makefile.inc,v 1.2 2009/07/21 01:00:41 mrg Exp $

.include <bsd.own.mk>

_CURDIR:= ${.CURDIR}

.include "${NETBSDSRCDIR}/lib/Makefile.inc"

.CURDIR:= ${_CURDIR}
