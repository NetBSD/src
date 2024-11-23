# $NetBSD: Makefile,v 1.11 2024/11/23 13:41:01 riastradh Exp $

.include <bsd.own.mk>

LIBEXECINFO_MACHINE_ARCH?= ${MACHINE_ARCH}
USE_UNWIND=yes
WARNS?=4
INCSDIR=/usr/include
INCS=execinfo.h

#CPPFLAGS+='-D__RCSID(a)=' -D_GNU_SOURCE '-D__printflike(a,b)='
#CPPFLAGS+=-I/usr/include/libelf
#COPTS+=-std=gnu99

LIBDPLIBS+= elf	${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf

LIB=execinfo
SRCS=symtab.c backtrace.c
MAN= backtrace.3

.if ${USE_UNWIND} == "yes"
SRCS+=unwind.c
.if !empty(LIBEXECINFO_MACHINE_ARCH:Mearm*)
SRCS+=unwind_arm_ehabi_stub.c
.endif
.else
SRCS+=builtin.c
.endif

EXPSYM_SRCS+=	execinfo.common.expsym

.if exists(${.CURDIR}/symbol_${LIBEXECINFO_MACHINE_ARCH}.c)
SRCS+=symbol_${LIBEXECINFO_MACHINE_ARCH}.c
EXPSYM_SRCS+=	execinfo.${LIBEXECINFO_MACHINE_ARCH}.expsym
.endif

LIB_EXPSYM=	execinfo.expsym
execinfo.expsym: ${EXPSYM_SRCS}
	${_MKTARGET_CREATE}
	LC_ALL=C sort -m ${.ALLSRC} >${.TARGET}.tmp && \
	${MV} ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	execinfo.expsym execinfo.expsym.tmp

MLINKS+= backtrace.3 backtrace_symbols.3
MLINKS+= backtrace.3 backtrace_symbols_fmt.3
MLINKS+= backtrace.3 backtrace_symbols_fd.3
MLINKS+= backtrace.3 backtrace_symbols_fd_fmt.3

.include <bsd.lib.mk>
