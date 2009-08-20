#	$NetBSD: Makefile,v 1.6 2009/08/20 15:14:49 tsutsui Exp $

BINDIR?= /sbin
WARNS?=	4

PROG=	binpatch
SRCS=	binpatch.c
SRCS+=	exec_aout.c exec_ecoff.c exec_elf32.c exec_elf64.c exec_coff.c

#MAN=	binpatch.8	# currently it's in src/share/man/man8/man8.atari
NOMAN=

MDSETIMAGE=${NETBSDSRCDIR}/usr.sbin/mdsetimage
CPPFLAGS+= -I${MDSETIMAGE}
.PATH:	${MDSETIMAGE}

.include <bsd.own.mk>	# for ${MACHINE_CPU}

.if	${MACHINE_ARCH} == "alpha"
CPPFLAGS+=-DNLIST_ECOFF
CPPFLAGS+=-DNLIST_ELF64
.elif	${MACHINE_CPU} == "mips"
CPPFLAGS+=-DNLIST_ECOFF
CPPFLAGS+=-DNLIST_ELF32
CPPFLAGS+=-DNLIST_AOUT
.elif	${MACHINE_ARCH} == "powerpc"
CPPFLAGS+=-DNLIST_ELF32
.elif	${MACHINE_ARCH} == "m68k" || \
	${MACHINE_ARCH} == "m68000" || \
	${MACHINE_ARCH} == "vax" || \
	${MACHINE_CPU} == "arm"
CPPFLAGS+=-DNLIST_ELF32
CPPFLAGS+=-DNLIST_AOUT
.elif	${MACHINE_CPU} == "sh3"
CPPFLAGS+=-DNLIST_COFF
CPPFLAGS+=-DNLIST_ELF32
.elif   ${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64"  || \
	${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=-DNLIST_ELF64
CPPFLAGS+=-DNLIST_ELF32
CPPFLAGS+=-DNLIST_AOUT
.else
#CPPFLAGS+=-DNLIST_AOUT
#CPPFLAGS+=-DNLIST_ECOFF
CPPFLAGS+=-DNLIST_ELF32
#CPPFLAGS+=-DNLIST_ELF64
#CPPFLAGS+=-DNLIST_COFF
.endif

.include <bsd.prog.mk>
