#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:06 joerg Exp $

LIB=	LLVMDebugInfoPDBDIA

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/PDB/DIA

SRCS+=	DIAEnumFrameData.cpp \
	DIAEnumInjectedSources.cpp \
	DIAEnumSectionContribs.cpp \
	DIAEnumTables.cpp \
	DIAFrameData.cpp \
	DIAInjectedSource.cpp \
	DIASectionContrib.cpp \
	DIATable.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
