#	$NetBSD: Makefile,v 1.16 2011/10/27 19:56:50 christos Exp $

LIB=	posix

.include <bsd.own.mk>

# This library replaces routines in libc; no sense in warning about it.
LLIBS=

CPPFLAGS+=-D_REENTRANT -I${NETBSDSRCDIR}/lib/libc/include
CPPFLAGS+=-I${NETBSDSRCDIR}/sys

.include "${.CURDIR}/../libc/libcincludes.mk"

AFLAGS+=-I${ARCHDIR}

.if ${MACHINE_ARCH} == "powerpc64"
# XXX: How come it does not resolve from libc?
.PATH.c: ${NETBSDSRCDIR}/lib/libc/gen
SRCS+= _errno.c
.endif

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
