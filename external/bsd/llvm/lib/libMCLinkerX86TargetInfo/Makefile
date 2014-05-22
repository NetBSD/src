#	$NetBSD: Makefile,v 1.1.12.2 2014/05/22 16:14:45 yamt Exp $

LIB=	MCLinkerX86TargetInfo

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/Target/X86/TargetInfo

SRCS=	X86TargetInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
