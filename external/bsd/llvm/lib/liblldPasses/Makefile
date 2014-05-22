#	$NetBSD: Makefile,v 1.2.4.2 2014/05/22 16:14:46 yamt Exp $

LIB=	lldPasses

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/Passes

SRCS+=	GOTPass.cpp \
	LayoutPass.cpp \
	RoundTripNativePass.cpp \
	RoundTripYAMLPass.cpp \
	StubsPass.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
