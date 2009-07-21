#	$NetBSD: Makefile.inc,v 1.1 2009/07/21 01:00:39 mrg Exp $

.include <bsd.own.mk>

_CURDIR:= ${.CURDIR}

.include "${NETBSDSRCDIR}/external/ibm-public/postfix/lib/Makefile.inc"

.CURDIR:= ${_CURDIR}
