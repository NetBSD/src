#	$NetBSD: Makefile,v 1.1.2.2 2018/07/28 04:37:10 pgoyette Exp $

LIB=	LLVMDebugInfoPDBDIA

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/DebugInfo/PDB/DIA

SRCS+=	DIAEnumInjectedSources.cpp \
	DIAEnumSectionContribs.cpp \
	DIAEnumTables.cpp \
	DIAInjectedSource.cpp \
	DIASectionContrib.cpp \
	DIATable.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
