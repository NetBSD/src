#	$NetBSD: Makefile,v 1.3.4.2 2014/08/19 23:51:27 tls Exp $

LIB=	lldPasses

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/Passes

SRCS+=	LayoutPass.cpp \
	RoundTripNativePass.cpp \
	RoundTripYAMLPass.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
