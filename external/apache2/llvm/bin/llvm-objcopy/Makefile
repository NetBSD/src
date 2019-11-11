#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:00 joerg Exp $

PROG_CXX=	llvm-objcopy
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-objcopy

CPPFLAGS+=	-I${LLVM_SRCDIR}/tools/llvm-objcopy

SRCS=	Buffer.cpp \
	CopyConfig.cpp \
	llvm-objcopy.cpp

.PATH: ${LLVM_SRCDIR}/tools/llvm-objcopy/COFF
SRCS+=	COFFObjcopy.cpp \
	Object.cpp \
	Reader.cpp \
	Writer.cpp

.PATH: ${LLVM_SRCDIR}/tools/llvm-objcopy/ELF
SRCS+=	ELFConfig.cpp \
	ELFObjcopy.cpp \
	ELFObject.cpp

ELFObject.cpp: ${LLVM_SRCDIR}/tools/llvm-objcopy/ELF/Object.cpp
	cat ${LLVM_SRCDIR}/tools/llvm-objcopy/ELF/Object.cpp > ELFObject.cpp

CPPFLAGS.ELFObject.cpp+=	-I${LLVM_SRCDIR}/tools/llvm-objcopy/ELF

.PATH: ${LLVM_SRCDIR}/tools/llvm-objcopy/MachO
SRCS+=	MachOLayoutBuilder.cpp \
	MachOObjcopy.cpp \
	MachOObject.cpp \
	MachOReader.cpp \
	MachOWriter.cpp

MachOObject.cpp: ${LLVM_SRCDIR}/tools/llvm-objcopy/MachO/Object.cpp
	cat ${LLVM_SRCDIR}/tools/llvm-objcopy/MachO/Object.cpp > MachOObject.cpp

CPPFLAGS.MachOObject.cpp+=	-I${LLVM_SRCDIR}/tools/llvm-objcopy/MachO

TABLEGEN_SRC=		CommonOpts.td ObjcopyOpts.td StripOpts.td

TABLEGEN_OUTPUT.CommonOpts.td= \
	CommonOpts.inc|-gen-opt-parser-defs

TABLEGEN_OUTPUT.ObjcopyOpts.td= \
	ObjcopyOpts.inc|-gen-opt-parser-defs

TABLEGEN_OUTPUT.StripOpts.td= \
	StripOpts.inc|-gen-opt-parser-defs
.include "${.PARSEDIR}/../../tablegen.mk"

LDADD+=	-lz
DPADD+=	${LIBZ}

LLVM_LIBS+= \
	Object \
	BitReader \
	BitstreamReader \
	MCParser \
	MC \
	IR \
	BinaryFormat \
	Option \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
