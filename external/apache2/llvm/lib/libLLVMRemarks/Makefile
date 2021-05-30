#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:54 joerg Exp $

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
