#	$NetBSD: Makefile,v 1.6 2000/12/18 08:51:39 scw Exp $

PROG=   crunchide
SRCS=	crunchide.c exec_aout.c exec_coff.c exec_ecoff.c exec_elf32.c \
	exec_elf64.c

.if	${MACHINE_ARCH} == "alpha"
CPPFLAGS+=-DNLIST_ELF64 -DNLIST_ECOFF
.elif	${MACHINE_ARCH} == "arm26" || \
	${MACHINE_ARCH} == "mips" || \
	${MACHINE_ARCH} == "powerpc" || \
	${MACHINE} == "mvme68k"
CPPFLAGS+=-DNLIST_ELF32
.elif	${MACHINE_ARCH} == "sh3"
CPPFLAGS+=-DNLIST_ELF32 -DNLIST_COFF
.elif	${MACHINE_ARCH} == "sparc64"
CPPFLAGS+=-DNLIST_AOUT -DNLIST_ELF32 -DNLIST_ELF64
.elif	${MACHINE_ARCH} == "sparc" || \
	${MACHINE_ARCH} == "i386"
CPPFLAGS+=-DNLIST_AOUT -DNLIST_ELF32
.else
CPPFLAGS+=-DNLIST_AOUT
.endif

.include <bsd.prog.mk>
