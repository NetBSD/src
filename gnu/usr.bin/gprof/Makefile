#	$NetBSD: Makefile,v 1.8 2001/08/06 20:01:23 tv Exp $

.include <bsd.own.mk>

.if !defined(USE_NEW_TOOLCHAIN)
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

TEXINFO=	gprof.texi

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}

.PATH:		${DIST}/gprof

.include <bsd.info.mk>
.endif

.include <bsd.prog.mk>
