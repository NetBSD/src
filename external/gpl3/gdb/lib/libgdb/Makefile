#	$NetBSD: Makefile,v 1.2.2.1 2012/04/17 00:04:30 yamt Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		gdb
GDBARCHDIR=	${.CURDIR}/arch/${MACHINE_ARCH}

.include "${GDBARCHDIR}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gdb6

SRCS=		${G_LIBGDB_OBS:.o=.c}
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS+=	-I${.CURDIR} \
		-I${GDBARCHDIR} \
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-I${DIST}/gdb/common \
		-I${DIST}/include/opcode \
		-I${DIST}/libdecnumber \
		-I${DIST} \
		-I${.CURDIR}/../libbfd/arch/${MACHINE_ARCH} \
		-I${.CURDIR}/../libdecnumber/arch/${MACHINE_ARCH} \
		-I${DIST}/bfd \
		-I${DIST}/include \
		-DTARGET_SYSTEM_ROOT=\"\" \
		-DTARGET_SYSTEM_ROOT_RELOCATABLE=0 \
		-DBINDIR=\"${BINDIR}\" \
		${GCPPFLAGS:M-D*}

# XXX inf-ptrace.c has some bogus (PTRACE_TYPE_ARG3) casts, but in 
# code that's never exercised on NetBSD.
CPPFLAGS.inf-ptrace.c=	-Wno-error

CWARNFLAGS.clang+=	-Wno-switch

.include "../../Makefile.inc"
.include <bsd.lib.mk>

.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/python ${DIST}/gdb/common ${DIST}/gdb/doc \
    ${GDBARCHDIR}

#
COPTS+=	-Wno-stack-protector

# Don't run yacc/bison or lex
.y.c .y.h .l.c:

.if ${MACHINE} == "amiga"
COPTS.dwarf2-frame.c+=-O1
.endif

.if defined(HAVE_GCC) && HAVE_GCC == 4 && ${MACHINE} == "sun2"
COPTS.dwarf2-frame.c+=-O0
.endif
