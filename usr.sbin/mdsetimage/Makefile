#	$NetBSD: Makefile,v 1.9 2001/05/01 20:35:19 matt Exp $

.include <bsd.own.mk>

PROG=	mdsetimage
SRCS=	mdsetimage.c exec_aout.c exec_ecoff.c exec_elf32.c exec_elf64.c \
	exec_coff.c
MAN=	mdsetimage.8

.if	${MACHINE_ARCH} == "alpha"
CPPFLAGS+=-DNLIST_ECOFF
CPPFLAGS+=-DNLIST_ELF64
.elif	${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
CPPFLAGS+=-DNLIST_ECOFF
CPPFLAGS+=-DNLIST_ELF32
CPPFLAGS+=-DNLIST_AOUT
.elif	${MACHINE_ARCH} == "powerpc"
CPPFLAGS+=-DNLIST_ELF32
.elif	${MACHINE_ARCH} == "i386" || \
	${MACHINE_ARCH} == "m68k" || \
	${MACHINE_ARCH} == "sparc" || \
	${MACHINE_ARCH} == "vax" || \
	${MACHINE_CPU} == "arm"
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
