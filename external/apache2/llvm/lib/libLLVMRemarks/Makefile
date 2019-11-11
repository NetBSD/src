#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:11 joerg Exp $

LIB=	LLVMRemarks

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Remarks

SRCS+=	BitstreamRemarkParser.cpp \
	BitstreamRemarkSerializer.cpp \
	Remark.cpp \
	RemarkFormat.cpp \
	RemarkParser.cpp \
	RemarkSerializer.cpp \
	RemarkStringTable.cpp \
	YAMLRemarkParser.cpp \
	YAMLRemarkSerializer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
