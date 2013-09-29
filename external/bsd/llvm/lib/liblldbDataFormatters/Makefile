#	$NetBSD: Makefile,v 1.1 2013/09/29 12:12:00 joerg Exp $

LIB=	lldbDataFormatters

.include <bsd.init.mk>

.PATH: ${LLDB_SRCDIR}/source/DataFormatters

SRCS+=	CF.cpp \
	Cocoa.cpp \
	CXXFormatterFunctions.cpp \
	DataVisualization.cpp \
	FormatCache.cpp \
	FormatClasses.cpp \
	FormatManager.cpp \
	LibCxx.cpp \
	LibCxxList.cpp \
	LibCxxMap.cpp \
	LibCxxUnorderedMap.cpp \
	LibStdcpp.cpp \
	NSArray.cpp \
	NSDictionary.cpp \
	NSSet.cpp \
	TypeCategory.cpp \
	TypeCategoryMap.cpp \
	TypeFormat.cpp \
	TypeSummary.cpp \
	TypeSynthetic.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
