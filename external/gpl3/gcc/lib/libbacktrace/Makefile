#	$NetBSD

DIST=		${NETBSDSRCDIR}/external/gpl3/gcc/dist
GNUHOSTDIST=    ${DIST}

LIBISPRIVATE=   yes

.include <bsd.own.mk>

LIB=		backtrace

SRCS=		dwarf.c elf.c fileline.c mmap.c mmapio.c nounwind.c \
		posix.c print.c state.c
		
CPPFLAGS+=	-I${DIST}/include -I${DIST}/libgcc
CPPFLAGS+=	-I${.CURDIR}/arch/${GCC_MACHINE_ARCH}

.include <bsd.lib.mk>

.PATH: ${DIST}/libbacktrace
