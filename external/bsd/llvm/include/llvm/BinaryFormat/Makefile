#	$NetBSD: Makefile,v 1.1 2018/12/31 19:33:03 maya Exp $

.include <bsd.init.mk>

SUBDIR+=	ELFRelocs

.PATH:	${LLVM_SRCDIR}/include/llvm/BinaryFormat

INCS=	\
	COFF.h \
	Dwarf.def \
	Dwarf.h \
	DynamicTags.def \
	ELF.h \
	ELFRelocs \
	MachO.def \
	MachO.h \
	Magic.h \
	Wasm.h \
	WasmRelocs.def

INCSDIR=	/usr/include/llvm/BinaryFormat

.include <bsd.subdir.mk>
.include <bsd.prog.mk>
