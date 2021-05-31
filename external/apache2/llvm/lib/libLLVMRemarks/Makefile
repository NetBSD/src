#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:09 cjep Exp $

LIB=	LLVMRemarks

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Remarks

SRCS+=	BitstreamRemarkParser.cpp \
	BitstreamRemarkSerializer.cpp \
	Remark.cpp \
	RemarkFormat.cpp \
	RemarkLinker.cpp \
	RemarkParser.cpp \
	RemarkSerializer.cpp \
	RemarkStreamer.cpp \
	RemarkStringTable.cpp \
	YAMLRemarkParser.cpp \
	YAMLRemarkSerializer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
