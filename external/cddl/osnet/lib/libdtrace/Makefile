#	$NetBSD: Makefile,v 1.16.12.1 2018/06/25 07:25:24 pgoyette Exp $

# $FreeBSD: head/cddl/lib/libdtrace/Makefile 314654 2017-03-04 11:30:04Z ngie $

WARNS=1
.include <bsd.init.mk>

LIB=		dtrace

SRCS=		dt_aggregate.c \
		dt_as.c \
		dt_buf.c \
		dt_cc.c \
		dt_cg.c \
		dt_consume.c \
		dt_decl.c \
		dt_dis.c \
		dt_dof.c \
		dt_error.c \
		dt_errtags.c \
		dt_grammar.y \
		dt_handle.c \
		dt_ident.c \
		dt_inttab.c \
		dt_isadep.c \
		dt_lex.l \
		dt_link.c \
		dt_list.c \
		dt_map.c \
		dt_module.c \
		dt_names.c \
		dt_open.c \
		dt_options.c \
		dt_parser.c \
		dt_pcb.c \
		dt_pid.c \
		dt_pq.c \
		dt_pragma.c \
		dt_print.c \
		dt_printf.c \
		dt_proc.c \
		dt_program.c \
		dt_provider.c \
		dt_regset.c \
		dt_string.c \
		dt_strtab.c \
		dt_subr.c \
		dt_sugar.c \
		dt_work.c \
		dt_xlator.c \
		gmatch.c

DSRCS=		errno.d			\
		psinfo.d		\
		signal.d		\
		unistd.d

FILES=		${DSRCS}
FILESDIR=	/usr/lib/dtrace

.PATH:		${.CURDIR}/../../dist/lib/libgen/common
.PATH:		${.CURDIR}/../../dist/lib/libdtrace/common

CPPFLAGS+=	-I${.OBJDIR} \
		-I${.CURDIR} \
		-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_USR_DISTDIR}/lib/libctf/common \
		-I${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common \
		-I${OPENSOLARIS_SYS_DISTDIR}/uts/common

COPTS.dt_aggregate.c	+= -Wno-stack-protector
COPTS.dt_consume.c	+= -Wno-stack-protector
COPTS.dt_decl.c		+= -Wno-stack-protector
COPTS.dt_dof.c		+= -Wno-stack-protector
COPTS.dt_handle.c	+= -Wno-stack-protector
COPTS.dt_ident.c	+= -Wno-stack-protector
COPTS.dt_link.c		+= -Wno-stack-protector
COPTS.dt_options.c	+= -Wno-stack-protector
COPTS.dt_parser.c	+= -Wno-stack-protector
COPTS.dt_pragma.c	+= -Wno-stack-protector
COPTS.dt_print.c	+= -Wno-stack-protector
COPTS.dt_printf.c	+= -Wno-stack-protector
COPTS.dt_program.c	+= -Wno-stack-protector
COPTS.dt_provider.c	+= -Wno-stack-protector
COPTS.dt_subr.c		+= -Wno-stack-protector

COPTS.dt_consume.c	+= -Wno-parentheses
COPTS.dt_consume.c	+= ${${ACTIVE_CC} == "gcc" :? -Wno-maybe-uninitialized :}
COPTS.dt_options.c	+= -Wno-parentheses
COPTS.dt_pid.c		+= ${${ACTIVE_CC} == "gcc" :? -Wno-unused-but-set-variable :}
COPTS.dt_isadep.c	+= -Wno-unused-variable

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-I${OPENSOLARIS_SYS_DISTDIR}/uts/intel
CPPFLAGS+=	-I${.CURDIR}/../../dev/dtrace/x86
.PATH:		${.CURDIR}/../../dist/lib/libdtrace/i386 ${.CURDIR}/../../dev/dtrace/x86
SRCS+=		dis_tables.c
COPTS.dis_tables.c	+= -Wno-parentheses
COPTS.dis_tables.c	+= ${${ACTIVE_CC} == "gcc" :? -Wno-maybe-uninitialized :}
.elif ${MACHINE_ARCH} == "sparc64"
CPPFLAGS+=	-I${OPENSOLARIS_SYS_DISTDIR}/uts/sparc
.elif ${MACHINE_CPU} == "arm"
CPPFLAGS+=	-I${OPENSOLARIS_SYS_DISTDIR}/uts/arm
.PATH:		${.CURDIR}/../../dist/lib/libdtrace/arm
.endif

LFLAGS+=-l

YFLAGS+=-d

CLEANFILES=	dt_errtags.c dt_names.c dt_grammar.h

MKERRTAGS=${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common/mkerrtags.sh 
dt_errtags.c: ${MKERRTAGS} ${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common/dt_errtags.h 
	sh ${MKERRTAGS} < ${.ALLSRC:M*.h} > ${.TARGET}

MKNAMES=${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common/mknames.sh
dt_names.c: ${MKNAMES} ${OPENSOLARIS_SYS_DISTDIR}/uts/common/sys/dtrace.h
	sh ${MKNAMES} < ${.ALLSRC:M*.h} > ${.TARGET}

beforedepend:	dt_errtags.c dt_names.c

.SUFFIXES: .in
.in:
	${CPP} -D_KERNEL ${CPPFLAGS} $< | tr -d ' ' | tr '"' '@' | \
	    ${TOOL_SED} -e 's/\&/\\\&/g' | grep '^s/' > ${.TARGET}


LIBDPLIBS=	proc	${NETBSDSRCDIR}/external/bsd/libproc/lib

.include <bsd.lib.mk>
