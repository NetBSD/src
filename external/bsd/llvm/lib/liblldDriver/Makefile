#	$NetBSD: Makefile,v 1.1 2013/09/02 14:34:57 joerg Exp $

LIB=	lldDriver

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/lib/Driver

SRCS+=	CoreDriver.cpp \
	DarwinLdDriver.cpp \
	Driver.cpp \
	GnuLdDriver.cpp \
	InputGraph.cpp \
	UniversalDriver.cpp \
	WinLinkDriver.cpp

TABLEGEN_SRC=	CoreOptions.td DarwinOptions.td LDOptions.td WinLinkOptions.td

TABLEGEN_OUTPUT.CoreOptions.td= \
	CoreOptions.inc|-gen-opt-parser-defs
TABLEGEN_OUTPUT.DarwinOptions.td= \
	DarwinOptions.inc|-gen-opt-parser-defs
TABLEGEN_OUTPUT.LDOptions.td= \
	LDOptions.inc|-gen-opt-parser-defs
TABLEGEN_OUTPUT.WinLinkOptions.td= \
	WinLinkOptions.inc|-gen-opt-parser-defs

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
