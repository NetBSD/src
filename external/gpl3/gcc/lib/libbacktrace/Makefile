#	$NetBSD: Makefile,v 1.3 2016/03/16 05:42:07 mrg Exp $

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
