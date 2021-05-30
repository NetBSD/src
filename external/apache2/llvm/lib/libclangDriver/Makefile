#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:56 joerg Exp $

LIB=	clangDriver

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/Driver

CPPFLAGS+=	-I${CLANG_SRCDIR}/lib/Driver

SRCS+=	Action.cpp \
	Compilation.cpp \
	DarwinSDKInfo.cpp \
	Distro.cpp \
	Driver.cpp \
	DriverOptions.cpp \
	Job.cpp \
	Multilib.cpp \
	OptionUtils.cpp \
	Phases.cpp \
	SanitizerArgs.cpp \
	Tool.cpp \
	ToolChain.cpp \
	Types.cpp \
	XRayArgs.cpp

.PATH: ${CLANG_SRCDIR}/lib/Driver/ToolChains

SRCS+=	AMDGPU.cpp \
	Ananas.cpp \
	AIX.cpp \
	AMDGPUOpenMP.cpp \
	AVR.cpp \
	BareMetal.cpp \
	Clang.cpp \
	CloudABI.cpp \
	CommonArgs.cpp \
	Contiki.cpp \
	CrossWindows.cpp \
	Cuda.cpp \
	Darwin.cpp \
	DragonFly.cpp \
	Flang.cpp \
	FreeBSD.cpp \
	Fuchsia.cpp \
	Gnu.cpp \
	Haiku.cpp \
	Hexagon.cpp \
	HIP.cpp \
	Hurd.cpp \
	InterfaceStubs.cpp \
	Linux.cpp \
	M68k.cpp \
	MinGW.cpp \
	Minix.cpp \
	MipsLinux.cpp \
	MSP430.cpp \
	MSVC.cpp \
	Myriad.cpp \
	NaCl.cpp \
	NetBSD.cpp \
	OpenBSD.cpp \
	PPCLinux.cpp \
	PS4CPU.cpp \
	RISCVToolchain.cpp \
	Solaris.cpp \
	TCE.cpp \
	VEToolchain.cpp \
	WebAssembly.cpp \
	XCore.cpp \
	ZOS.cpp

.PATH: ${CLANG_SRCDIR}/lib/Driver/ToolChains/Arch

SRCS+=	AArch64.cpp \
	ARM.cpp \
	Mips.cpp \
	PPC.cpp \
	RISCV.cpp \
	Sparc.cpp \
	SystemZ.cpp \
	VE.cpp \
	X86.cpp




.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
