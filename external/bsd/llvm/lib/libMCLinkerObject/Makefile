#	$NetBSD: Makefile,v 1.1.12.2 2014/05/22 16:14:45 yamt Exp $

LIB=	MCLinkerObject

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/Object

SRCS=	ObjectBuilder.cpp \
	ObjectLinker.cpp \
	SectionMap.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
