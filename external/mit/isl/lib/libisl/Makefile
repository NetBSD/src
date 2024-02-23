#	$NetBSD: Makefile,v 1.1 2024/02/23 05:33:04 mrg Exp $

.include <bsd.init.mk>

DIST=	${.CURDIR}/../../dist

# We don't install isl, just provide it for GCC.
LIBISPRIVATE?= yes

.if ${LIBISPRIVATE} != "yes"
INCS=	aff.h \
	aff_type.h \
	arg.h \
	ast.h \
	ast_build.h \
	ast_type.h \
	constraint.h \
	cpp.h \
	ctx.h \
	fixed_box.h \
	flow.h \
	hash.h \
	hmap.h \
	hmap_templ.c \
	id.h \
	id_to_ast_expr.h \
	id_to_id.h \
	id_to_pw_aff.h \
	id_type.h \
	ilp.h \
	list.h \
	local_space.h \
	lp.h \
	map.h \
	map_to_basic_set.h \
	map_type.h \
	mat.h \
	maybe.h \
	maybe_ast_expr.h \
	maybe_basic_set.h \
	maybe_id.h \
	maybe_pw_aff.h \
	maybe_templ.h \
	multi.h \
	obj.h \
	options.h \
	point.h \
	polynomial.h \
	polynomial_type.h \
	printer.h \
	printer_type.h \
	schedule.h \
	schedule_node.h \
	schedule_type.h \
	set.h \
	set_type.h \
	space.h \
	space_type.h \
	stream.h \
	stride_info.h \
	typed_cpp.h \
	union_map.h \
	union_map_type.h \
	union_set.h \
	union_set_type.h \
	val.h \
	val_gmp.h \
	val_type.h \
	vec.h \
	version.h \
	vertices.h
INCSDIR=      /usr/include/isl
.PATH: ${DIST}/include/isl

SUBDIR=	../../include
.include <bsd.subdir.mk>

.endif

LIB=	isl

LIBDPLIBS+= gmp ${.CURDIR}/../../../../lgpl3/gmp/lib/libgmp

SRCS=	\
	isl_gmp.c \
	isl_val_gmp.c \
	isl_aff.c \
	isl_aff_map.c \
	isl_affine_hull.c \
	isl_arg.c \
	isl_ast.c \
	isl_ast_build.c \
	isl_ast_build_expr.c \
	isl_ast_codegen.c \
	isl_ast_graft.c \
	basis_reduction_tab.c \
	isl_bernstein.c \
	isl_blk.c \
	isl_bound.c \
	isl_box.c \
	isl_coalesce.c \
	isl_constraint.c \
	isl_convex_hull.c \
	isl_ctx.c \
	isl_deprecated.c \
	isl_dim_map.c \
	isl_equalities.c \
	isl_factorization.c \
	isl_farkas.c \
	isl_ffs.c \
	isl_flow.c \
	isl_fold.c \
	isl_hash.c \
	isl_id_to_ast_expr.c \
	isl_id_to_id.c \
	isl_id_to_pw_aff.c \
	isl_ilp.c \
	isl_input.c \
	isl_local.c \
	isl_local_space.c \
	isl_lp.c \
	isl_map.c \
	isl_map_list.c \
	isl_map_simplify.c \
	isl_map_subtract.c \
	isl_map_to_basic_set.c \
	isl_mat.c \
	isl_morph.c \
	isl_id.c \
	isl_obj.c \
	isl_options.c \
	isl_output.c \
	isl_point.c \
	isl_polynomial.c \
	isl_printer.c \
	print.c \
	isl_range.c \
	isl_reordering.c \
	isl_sample.c \
	isl_scan.c \
	isl_schedule.c \
	isl_schedule_band.c \
	isl_schedule_node.c \
	isl_schedule_read.c \
	isl_schedule_tree.c \
	isl_schedule_constraints.c \
	isl_scheduler.c \
	isl_scheduler_clustering.c \
	isl_scheduler_scc.c \
	isl_set_list.c \
	isl_sort.c \
	isl_space.c \
	isl_stream.c \
	isl_seq.c \
	isl_set_to_ast_graft_list.c \
	isl_stride.c \
	isl_tab.c \
	isl_tab_pip.c \
	isl_tarjan.c \
	isl_transitive_closure.c \
	isl_union_map.c \
	isl_val.c \
	isl_vec.c \
	isl_version.c \
	isl_vertices.c

CPPFLAGS+=	-I${.CURDIR} -I${DIST} -I${DIST}/include -I${.CURDIR}/../../include

# Find <gmp.h>
GMPDIR=${NETBSDSRCDIR}/external/lgpl3/gmp
.include "${GMPDIR}/Makefile.arch"
CPPFLAGS+=	-I${GMPDIR}/lib/libgmp/arch/${GMP_MACHINE_ARCH}

.include <bsd.lib.mk>

.PATH: ${DIST}
