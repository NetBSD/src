#	$NetBSD: Makefile,v 1.5 2001/01/04 09:44:22 itojun Exp $

PROG=	mdsetimage
SRCS=	mdsetimage.c exec_aout.c exec_ecoff.c exec_elf32.c exec_elf64.c \
	exec_coff.c
MAN=	mdsetimage.8

.if	${MACHINE_ARCH} == "alpha"
CPPFLAGS+=-DNLIST_ECOFF
CPPFLAGS+=-DNLIST_ELF64
.elif	${MACHINE_ARCH} == "mips"
CPPFLAGS+=-DNLIST_ECOFF
CPPFLAGS+=-DNLIST_ELF32
CPPFLAGS+=-DNLIST_AOUT
.elif	${MACHINE_ARCH} == "powerpc" || \
	${MACHINE} == "mvme68k"
CPPFLAGS+=-DNLIST_ELF32
.elif	${MACHINE_ARCH} == "i386" || \
	${MACHINE_ARCH} == "sparc"
CPPFLAGS+=-DNLIST_ELF32
CPPFLAGS+=-DNLIST_AOUT
.elif	${MACHINE_ARCH} == "sh3el" || ${MACHINE_ARCH} == "sh3eb"
CPPFLAGS+=-DNLIST_COFF
CPPFLAGS+=-DNLIST_ELF32
.else
CPPFLAGS+=-DNLIST_AOUT
#CPPFLAGS+=-DNLIST_ECOFF
#CPPFLAGS+=-DNLIST_ELF32
#CPPFLAGS+=-DNLIST_ELF64
#CPPFLAGS+=-DNLIST_COFF
.endif

.include <bsd.prog.mk>
