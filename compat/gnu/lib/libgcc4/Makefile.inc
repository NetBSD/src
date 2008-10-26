#	$NetBSD: Makefile.inc,v 1.1 2008/10/26 07:38:28 mrg Exp $

_CURDIR:= ${.CURDIR}

.include "${NETBSDSRCDIR}/gnu/lib/libgcc4/Makefile.inc"

.CURDIR:= ${_CURDIR}
