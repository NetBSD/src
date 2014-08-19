#	$NetBSD: Makefile,v 1.6.6.2 2014/08/19 23:52:23 tls Exp $

# $FreeBSD: src/cddl/lib/libdtrace/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $

.include <bsd.own.mk>

.include	"../../Makefile.inc"

LIB=		dtrace

#WARNS=		1

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
		dt_pragma.c \
		dt_printf.c \
		dt_proc.c \
		dt_program.c \
		dt_provider.c \
		dt_regset.c \
		dt_string.c \
		dt_strtab.c \
		dt_subr.c \
		dt_work.c \
		dt_xlator.c \
		gmatch.c

DSRCS=		errno.d			\
		psinfo.d		\
		signal.d		\
		unistd.d

.PATH:		${.CURDIR}/../../dist/lib/libgen/common
.PATH:		${.CURDIR}/../../dist/lib/libdtrace/common

CPPFLAGS+=	-I${.OBJDIR} \
		-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_USR_DISTDIR}/lib/libctf/common \
		-I${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common \
		-I${OPENSOLARIS_SYS_DISTDIR}/uts/common

# XXX need new libelf (not GPL)

#CFLAGS+=	-DYYDEBUG

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
COPTS.dt_printf.c	+= -Wno-stack-protector
COPTS.dt_program.c	+= -Wno-stack-protector
COPTS.dt_provider.c	+= -Wno-stack-protector
COPTS.dt_subr.c		+= -Wno-stack-protector

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-I${OPENSOLARIS_SYS_DISTDIR}/uts/intel
.elif ${MACHINE_ARCH} == "sparc64"
CPPFLAGS+=	-I${OPENSOLARIS_SYS_DISTDIR}/uts/sparc
.elif ${MACHINE_CPU} == "arm"
CPPFLAGS+=	-I${OPENSOLARIS_SYS_DISTDIR}/uts/arm
.PATH:		${.CURDIR}/../../dist/lib/libdtrace/arm
SRCS+=	dt_isadep.c
.endif

LFLAGS+=-l

YFLAGS+=-d

CLEANFILES=	dt_errtags.c dt_names.c dt_grammar.h

dt_errtags.c:
	sh ${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common/mkerrtags.sh < ${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common/dt_errtags.h > dt_errtags.c

dt_names.c:
	sh ${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common/mknames.sh < ${OPENSOLARIS_SYS_DISTDIR}/uts/common/sys/dtrace.h > dt_names.c

beforedepend:	dt_errtags.c dt_names.c

foo:
	echo ${OPENSOLARIS_USR_DISTDIR}

#beforeinstall:
#.if exists(${DESTDIR}/usr/lib/dtrace)
#.for file in ${DSRCS}
#	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m ${NOBINMODE} ${.CURDIR}/${file} ${DESTDIR}/usr/lib/dtrace
#.endfor
#.endif

.include <bsd.lib.mk>
