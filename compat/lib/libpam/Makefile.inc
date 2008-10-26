#	$NetBSD: Makefile.inc,v 1.1 2008/10/26 07:38:32 mrg Exp $

_CURDIR:= ${.CURDIR}

.include "${NETBSDSRCDIR}/lib/libpam/Makefile.inc"

.CURDIR:= ${_CURDIR}
