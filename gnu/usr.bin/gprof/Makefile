#	$NetBSD: Makefile,v 1.4 1999/02/03 00:15:09 tv Exp $

TOP=	${.CURDIR}/../..
dist=	${TOP}/dist
srcdir=	${dist}/gprof

PROG=	gprof
SRCS=	basic_blocks.c call_graph.c cg_arcs.c cg_dfn.c cg_print.c core.c \
	gmon_io.c gprof.c hertz.c hist.c source.c search_list.c symtab.c \
	sym_ids.c utils.c flat_bl.c bsd_callg_bl.c fsf_callg_bl.c i386.c \
	alpha.c vax.c tahoe.c sparc.c

.PATH:	${srcdir}

CPPFLAGS+= -I${TOP}/lib/bfd -I${dist}/bfd -I${dist}/include -I${.CURDIR}
CPPFLAGS+= -I${DESTDIR}/usr/include/bfd
CPPFLAGS+= -DHAVE_CONFIG_H

DPADD=	${LIBBFD}
LDADD=	-lbfd

.include <bsd.prog.mk>
