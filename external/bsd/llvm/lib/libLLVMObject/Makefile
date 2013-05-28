#	$NetBSD: Makefile,v 1.6 2013/05/28 17:37:35 joerg Exp $

LIB=	LLVMObject
NOGCCERROR=	yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Object

SRCS+=	Archive.cpp \
	Binary.cpp \
	COFFObjectFile.cpp \
	COFFYAML.cpp \
	ELFObjectFile.cpp \
	Error.cpp \
	MachOObjectFile.cpp \
	Object.cpp \
	ObjectFile.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
