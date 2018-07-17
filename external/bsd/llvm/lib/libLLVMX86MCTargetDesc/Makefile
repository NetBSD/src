#	$NetBSD: Makefile,v 1.9 2018/07/17 18:55:29 joerg Exp $

LIB=	LLVMX86MCTargetDesc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/MCTargetDesc

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/X86

SRCS+=	X86AsmBackend.cpp \
	X86ELFObjectWriter.cpp \
	X86MachObjectWriter.cpp \
	X86MCAsmInfo.cpp \
	X86MCCodeEmitter.cpp \
	X86MCTargetDesc.cpp \
	X86WinCOFFObjectWriter.cpp \
	X86WinCOFFStreamer.cpp \
	X86WinCOFFTargetStreamer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
