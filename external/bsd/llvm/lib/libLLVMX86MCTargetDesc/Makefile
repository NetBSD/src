#	$NetBSD: Makefile,v 1.6.4.1 2014/08/10 07:09:10 tls Exp $

LIB=	LLVMX86MCTargetDesc

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/MCTargetDesc

X86_OBJDIR!=	cd ${.CURDIR}/../libLLVMX86CodeGen && ${PRINTOBJDIR}
CPPFLAGS+=	-I${X86_OBJDIR} -I${LLVM_SRCDIR}/lib/Target/X86

SRCS+=	X86AsmBackend.cpp \
	X86ELFObjectWriter.cpp \
	X86MCAsmInfo.cpp \
	X86MCCodeEmitter.cpp \
	X86MCTargetDesc.cpp \
	X86MachORelocationInfo.cpp \
	X86MachObjectWriter.cpp \
	X86ELFRelocationInfo.cpp \
	X86WinCOFFObjectWriter.cpp \
	X86WinCOFFStreamer.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
