#	$NetBSD: Makefile,v 1.1.12.2 2014/05/22 16:14:44 yamt Exp $

LIB=	MCLinkerARMTargetInfo

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/Target/ARM/TargetInfo

SRCS=	ARMTargetInfo.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
