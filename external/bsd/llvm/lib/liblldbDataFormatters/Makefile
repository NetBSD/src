#	$NetBSD: Makefile,v 1.2.6.1 2015/06/04 20:05:46 snj Exp $

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
	LibCxxInitializerList.cpp \
	LibCxxList.cpp \
	LibCxxMap.cpp \
	LibCxxUnorderedMap.cpp \
	LibCxxVector.cpp \
	LibStdcpp.cpp \
	NSArray.cpp \
	NSDictionary.cpp \
	NSIndexPath.cpp \
	NSSet.cpp \
	StringPrinter.cpp \
	TypeCategory.cpp \
	TypeCategoryMap.cpp \
	TypeFormat.cpp \
	TypeSummary.cpp \
	TypeSynthetic.cpp \
	ValueObjectPrinter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
