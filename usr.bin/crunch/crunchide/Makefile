#	$NetBSD: Makefile,v 1.11 2001/06/30 14:52:42 bjh21 Exp $

.include <bsd.own.mk>

PROG=   crunchide
SRCS=	crunchide.c exec_aout.c exec_coff.c exec_ecoff.c exec_elf32.c \
	exec_elf64.c

.if	${MACHINE_ARCH} == "alpha"
CPPFLAGS+=-DNLIST_ELF64 -DNLIST_ECOFF
.elif	${MACHINE_ARCH} == "mips" || \
	${MACHINE_ARCH} == "mipsel" || \
	${MACHINE_ARCH} == "mipseb" || \
	${MACHINE_ARCH} == "powerpc"
CPPFLAGS+=-DNLIST_ELF32
.elif	${MACHINE_ARCH} == "sh3eb" || ${MACHINE_ARCH} == "sh3el"
CPPFLAGS+=-DNLIST_ELF32 -DNLIST_COFF
.elif	${MACHINE_ARCH} == "sparc64"
CPPFLAGS+=-DNLIST_AOUT -DNLIST_ELF32 -DNLIST_ELF64
.elif	${MACHINE_CPU} == "arm" || \
	${MACHINE_ARCH} == "i386" || \
	${MACHINE_ARCH} == "m68k" || \
	${MACHINE_ARCH} == "sparc"
CPPFLAGS+=-DNLIST_AOUT -DNLIST_ELF32
.else
CPPFLAGS+=-DNLIST_AOUT
.endif

.include <bsd.prog.mk>
