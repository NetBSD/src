#	$NetBSD: Makefile,v 1.10 2011/04/14 08:21:06 mrg Exp $

PROG=		crash
MAN=		crash.8
RUMPKERNEL=	yes	# XXX: Avoid -mcmodel=kernel

LDADD+=	-lutil -lkvm -ledit -lterminfo -T${.CURDIR}/ldscript.crash
DPADD+=	${LIBUTIL} ${LIBKVM} ${LIBEDIT} ${LIBTERMINFO}

# some ddb kernel components need limited modifications.  for now,
# punt if not noted as implemented here.
.if    ${MACHINE} == "amd64" \
    || ${MACHINE} == "i386" \
    || ${MACHINE} == "sparc64"
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
SRCS+=	db_access.c db_elf.c db_examine.c
SRCS+=	db_expr.c db_lex.c db_output.c db_print.c
SRCS+=	db_sym.c db_variables.c db_write_cmd.c

.PATH:	${S}/arch/${MACHINE}/${MACHINE}
SRCS+=	db_disasm.c

. if    ${MACHINE} == "amd64" \
     || ${MACHINE} == "i386"
.PATH:	${S}/arch/x86/x86
SRCS+=	db_trace.c
SRCS+=	db_machdep.c
. endif

. if ${MACHINE} == "sparc64"
SRCS+=	db_trace.c db_interface.c
. endif

# crash main source
SRCS+=	crash.c

# arch.c
.PATH:	${.CURDIR}/arch
. if (exists(${.CURDIR}/arch/${MACHINE_ARCH}.c))
SRCS+=	${MACHINE_ARCH}.c
. else
SRCS+=	generic.c
. endif

# vers.c
SRCS+=	vers.c
vers.c:	${S}/conf/newvers.sh
	${HOST_SH} ${S}/conf/newvers.sh -r
CLEANFILES+=	vers.c version

.else				# } {

SRCS+=	unsupported.c

.endif				# }

.include <bsd.prog.mk>
.include <bsd.klinks.mk>
