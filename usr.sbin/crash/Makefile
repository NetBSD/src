#	$NetBSD: Makefile,v 1.48 2023/06/03 09:09:21 lukem Exp $

PROG=		crash
MAN=		crash.8
RUMPKERNEL=	yes	# XXX: Avoid -mcmodel=kernel

CWARNFLAGS.clang+=	-Wno-format

LDADD+=	-lutil -lkvm -ledit -lterminfo
DPADD+=	${LIBUTIL} ${LIBKVM} ${LIBEDIT} ${LIBTERMINFO}

.include <bsd.own.mk>

# some ddb kernel components need limited modifications.  for now,
# punt if not noted as implemented here.
.if    ${MACHINE} == "amd64" \
    || ${MACHINE} == "hppa" \
    || ${MACHINE} == "i386" \
    || ${MACHINE} == "sparc" \
    || ${MACHINE} == "sparc64" \
    || ${MACHINE_CPU} == "arm" \
    || ${MACHINE_CPU} == "aarch64" \
    || ${MACHINE_CPU} == "mips" \
    || ${MACHINE_ARCH} == "m68k" \
    || ${MACHINE_ARCH} == "powerpc" \
    || ${MACHINE_ARCH} == "powerpc64"
SRCS+=	db_trace.c
.if ${MACHINE_CPU} == "mips"
SRCS+=	db_interface.c mips_stacktrace.c
CPPFLAGS+=	-DDDB
.elif ${MACHINE_ARCH} != "m68k" \
   && ${MACHINE_ARCH} != "powerpc" \
   && ${MACHINE_ARCH} != "powerpc64"
SRCS+=	db_machdep.c
.endif
REALCRASH=yes
.else
REALCRASH=no
.endif

.if ${REALCRASH} != "no"	# {

S=		${.CURDIR}/../../sys

CPPFLAGS+=	-I${.CURDIR} -I${.OBJDIR} -I${S} -fno-strict-aliasing
CPPFLAGS+=	-DDDB_VERBOSE_HELP -DDB_MAX_LINE=10000000 -D_KMEMUSER
CPPFLAGS+=	-UDB_MACHINE_COMMANDS

# ddb files from kernel
.PATH:	$S/ddb
SRCS+=	db_command.c db_lwp.c db_proc.c db_xxx.c db_cpu.c
SRCS+=  db_autoconf.c
SRCS+=	db_access.c db_elf.c db_examine.c
SRCS+=	db_expr.c db_lex.c db_output.c db_print.c
SRCS+=	db_sym.c db_variables.c db_write_cmd.c

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
SRCS+=	db_disasm.c

.PATH:  $S/kern
SRCS+=	kern_timeout.c
CPPFLAGS.kern_timeout.c += -DCRASH -DDDB

CPPFLAGS +=-DLOCKDEBUG
SRCS += subr_lockdebug.c
CPPFLAGS.subr_lockdebug.c += -DCRASH -DDDB

CPPFLAGS+=	-I${S}/arch

. if    ${MACHINE} == "amd64" \
     || ${MACHINE} == "i386"
MACHINE_FAMILY = x86
. elif  ${MACHINE} == "sparc64"
MACHINE_FAMILY = sparc
. else
MACHINE_FAMILY = ${MACHINE_CPU}
. endif

.if ${MACHINE_CPU} == "arm"
.PATH:	${S}/arch/arm/arm32
SRCS+=disassem.c cpufunc_asm.S
.endif
.if ${MACHINE_CPU} == "aarch64"
SRCS+=disasm.c
.endif

.PATH:	${S}/arch/${MACHINE_FAMILY}/${MACHINE_FAMILY}

# crash main source
SRCS+=	crash.c

# arch.c
.PATH:	${.CURDIR}/arch
. if (exists(${.CURDIR}/arch/${MACHINE_FAMILY}.c))
SRCS+=	${MACHINE_FAMILY}.c
. else
SRCS+=	generic.c
. endif

COPTS.db_output.c += -Wno-format-nonliteral

.if ${MKREPRO} == "yes"
.       if ${MKREPRO_TIMESTAMP:U0} != 0
NVFLAGS+=-r ${MKREPRO_TIMESTAMP}
.       else
NVFLAGS+=-R
.       endif
.endif

# vers.c
SRCS+=	vers.c
vers.c:	${S}/conf/newvers.sh ${_NETBSD_VERSION_DEPENDS}
	${HOST_SH} ${S}/conf/newvers.sh ${NVFLAGS} -n -m ${MACHINE} -i CRASH
CLEANFILES+=	vers.c version

.else				# } {

SRCS+=	unsupported.c

.endif				# }

.if  ${MACHINE} == "sparc" \
    || ${MACHINE} == "sparc64"
COPTS.kern_timeout.c += -Wno-stack-protector
.endif

.include "../../compat/exec.mk"

COPTS.db_command.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

COPTS+=		-fcommon

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
