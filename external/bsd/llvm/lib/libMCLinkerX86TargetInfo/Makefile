#	$NetBSD: Makefile,v 1.1.6.2 2013/06/23 06:28:25 tls Exp $

LIB=	MCLinkerX86TargetInfo

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/Target/X86/TargetInfo

SRCS=	X86TargetInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
