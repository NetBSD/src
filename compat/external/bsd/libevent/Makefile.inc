#	$NetBSD: Makefile.inc,v 1.1 2009/11/02 10:03:57 plunky Exp $

.include <bsd.own.mk>

_CURDIR:= ${.CURDIR}

.include "${NETBSDSRCDIR}/external/bsd/libevent/Makefile.inc"

.CURDIR:= ${_CURDIR}
