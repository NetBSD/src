#	$NetBSD: Makefile,v 1.5 1999/02/09 18:35:39 tv Exp $

PROG=		gprof
SRCS=		basic_blocks.c call_graph.c cg_arcs.c cg_dfn.c cg_print.c \
		core.c gmon_io.c gprof.c hertz.c hist.c source.c \
		search_list.c symtab.c sym_ids.c utils.c flat_bl.c \
		bsd_callg_bl.c fsf_callg_bl.c \
		i386.c alpha.c vax.c sparc.c mips.c

CPPFLAGS+=	-I${.CURDIR} -I${BFDOBJ} -I${.CURDIR}/../../lib/libbfd \
		-I${DIST}/bfd -I${DIST}/include
LDADD=		-L${BFDOBJ} -lbfd
DPADD=		${BFDOBJ}/libbfd_pic.a

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${MAKE} print-objdir

.PATH:		${DIST}/gprof

.include <bsd.prog.mk>
