#	$NetBSD: Makefile,v 1.3 1998/10/19 04:15:10 ross Exp $

# Override this to build a cross profiler.
TARGET_ARCH?=${MACHINE_ARCH}

TOP=	${.CURDIR}/../..
dist=	${TOP}/dist
srcdir=	${dist}/gprof

PROG=	gprof
SRCS=	${TARGET_ARCH}.c basic_blocks.c call_graph.c \
	cg_arcs.c cg_dfn.c cg_print.c core.c \
	gmon_io.c gprof.c hertz.c hist.c source.c search_list.c symtab.c \
	sym_ids.c utils.c

SRCS+=	flat_bl.c bsd_callg_bl.c fsf_callg_bl.c

.PATH:	${srcdir}

CPPFLAGS+= -I${TOP}/lib/bfd
CPPFLAGS+= -I${dist}/bfd
CPPFLAGS+= -I${dist}/include
CPPFLAGS+= -I${DESTDIR}/usr/include/bfd
CPPFLAGS+= -DBSD44_FORMAT=1 -DTARGET_${TARGET_ARCH}
CPPFLAGS+= -DMACHINE_H=\"${TARGET_ARCH}.h\"

DPADD=	${LIBBFD}
LDADD=	-lbfd

.include <bsd.prog.mk>
