#	$NetBSD: Makefile,v 1.2 2013/09/21 22:28:12 joerg Exp $

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

TABLEGEN_SRC=	CoreOptions.td DarwinLdOptions.td GnuLdOptions.td WinLinkOptions.td

TABLEGEN_OUTPUT.CoreOptions.td= \
	CoreOptions.inc|-gen-opt-parser-defs
TABLEGEN_OUTPUT.DarwinLdOptions.td= \
	DarwinLdOptions.inc|-gen-opt-parser-defs
TABLEGEN_OUTPUT.GnuLdOptions.td= \
	GnuLdOptions.inc|-gen-opt-parser-defs
TABLEGEN_OUTPUT.WinLinkOptions.td= \
	WinLinkOptions.inc|-gen-opt-parser-defs

.include "${.PARSEDIR}/../../tablegen.mk"

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
