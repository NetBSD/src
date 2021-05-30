#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:57 joerg Exp $

LIB=	clangSerialization

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Serialization

SRCS+=	ASTCommon.cpp \
	ASTReader.cpp \
	ASTReaderDecl.cpp \
	ASTReaderStmt.cpp \
	ASTWriter.cpp \
	ASTWriterDecl.cpp \
	ASTWriterStmt.cpp \
	GeneratePCH.cpp \
	GlobalModuleIndex.cpp \
	InMemoryModuleCache.cpp \
	ModuleFile.cpp \
	ModuleFileExtension.cpp \
	ModuleManager.cpp \
	PCHContainerOperations.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
