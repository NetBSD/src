#	$NetBSD: Makefile,v 1.3 2009/06/04 12:19:35 christos Exp $

PROG=	crash
MAN=	crash.8
RUMPKERNEL=	yes	# XXX: Avoid -mcmodel=kernel

LDADD+=	-lkvm -ledit -ltermcap
DPADD+=	${LIBKVM} ${LIBEDIT} ${LIBTERMCAP}

# some ddb kernel components need limited modifications.  for now,
# punt if not noted as implemented here.
.if (${MACHINE} != "i386")

SRCS+=	unsupported.c

.else

S=	${.CURDIR}/../../sys

CPPFLAGS+=	-I${.CURDIR} -I${.OBJDIR} -I${S} -fno-strict-aliasing
CPPFLAGS+=	-DDDB_VERBOSE_HELP -DDB_MAX_LINE=10000000 -D_KMEMUSER

# ddb files from kernel
.PATH:	$S/ddb
SRCS+=	db_command.c db_lwp.c db_proc.c db_xxx.c db_cpu.c
SRCS+=	db_access.c db_elf.c db_examine.c
SRCS+=	db_expr.c db_lex.c db_output.c db_print.c
SRCS+=	db_sym.c db_variables.c db_write_cmd.c

# db_trace.c, db_disasm.c
.PATH:	${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}
.for i in ${i} db_disasm db_trace
. if (exists(${S}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}/${i}.c))
SRCS+=	${i}.c
. endif
.endfor

# crash main source
SRCS+=	crash.c

# arch.c
.PATH:	${.CURDIR}/arch
.if (exists(${.CURDIR}/arch/${MACHINE_ARCH}.c))
SRCS+=	${MACHINE_ARCH}.c
.else
SRCS+=	generic.c
.endif

# vers.c
SRCS+=	vers.c
vers.c:	${S}/conf/newvers.sh
	${HOST_SH} ${S}/conf/newvers.sh
CLEANFILES+=	vers.c version

.endif

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
