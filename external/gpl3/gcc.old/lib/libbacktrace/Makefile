#	$NetBSD: Makefile,v 1.4 2018/11/11 22:49:47 mrg Exp $

LIBISPRIVATE=   yes

.include <bsd.init.mk>

LIB=		backtrace

DIST=		${GCCDIST}
GNUHOSTDIST=    ${DIST}

SRCS=		dwarf.c elf.c fileline.c mmap.c mmapio.c nounwind.c \
		posix.c print.c state.c
		
CPPFLAGS+=	-I${DIST}/include -I${DIST}/libgcc
CPPFLAGS+=	-I${.CURDIR}/arch/${GCC_MACHINE_ARCH}

.include <bsd.lib.mk>

.PATH: ${DIST}/libbacktrace
