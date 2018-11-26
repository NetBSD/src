#	$NetBSD: Makefile,v 1.3.2.1 2018/11/26 01:51:44 pgoyette Exp $

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
