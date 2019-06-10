#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 21:50:52 christos Exp $

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
