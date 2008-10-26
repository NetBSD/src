#	$NetBSD: Makefile,v 1.14 2008/10/26 07:28:49 mrg Exp $

LIB=	posix

.include <bsd.own.mk>

# This library replaces routines in libc; no sense in warning about it.
LLIBS=

CPPFLAGS+=-D_REENTRANT -I${NETBSDSRCDIR}/lib/libc/include
CPPFLAGS+=-I${NETBSDSRCDIR}/sys

.include "${.CURDIR}/../libc/libcincludes.mk"

AFLAGS+=-I${ARCHDIR}

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
