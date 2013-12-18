#	$NetBSD: Makefile,v 1.8.12.1 2013/12/18 18:43:30 matt Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		gdb

.include "${.CURDIR}/../arch/${GDB_MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gdb6

SRCS=		${G_LIBGDB_OBS:.o=.c}
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS=	-I${.CURDIR} \
		-I${.CURDIR}/../arch/${GDB_MACHINE_ARCH} \
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-I${DIST}/include/opcode \
		-I${DIST} \
		-I${.CURDIR}/../bfd/arch/${GDB_MACHINE_ARCH} \
		-I${DIST}/bfd \
		-I${DIST}/include \
		${GCPPFLAGS:M-D*}

# XXX inf-ptrace.c has some bogus (PTRACE_TYPE_ARG3) casts, but in 
# code that's never exercised on NetBSD.
CPPFLAGS.inf-ptrace.c=	-Wno-error

.include "../../Makefile.inc"
.include <bsd.lib.mk>

.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/doc ${.CURDIR}/../arch/${GDB_MACHINE_ARCH}

#
COPTS+=	-Wno-stack-protector

# Don't run yacc/bison or lex
.y.c .y.h .l.c:
