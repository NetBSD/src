#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:12 cjep Exp $

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
