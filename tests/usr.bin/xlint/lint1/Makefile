# $NetBSD: Makefile,v 1.30 2021/01/17 23:00:41 rillig Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/xlint/lint1

TESTS_SH=	t_integration

FILESDIR=	${TESTSDIR}
FILES+=		d_alignof.c
FILES+=		d_bltinoffsetof.c
FILES+=		d_c99_bool.c
FILES+=		d_c99_bool.exp
FILES+=		d_c99_bool_strict.c
FILES+=		d_c99_bool_strict.exp
FILES+=		d_c99_bool_strict_syshdr.c
FILES+=		d_c99_bool_strict_syshdr.exp
FILES+=		d_c99_anon_struct.c
FILES+=		d_c99_anon_union.c
FILES+=		d_c99_complex_num.c
FILES+=		d_c99_complex_split.c
FILES+=		d_c99_compound_literal_comma.c
FILES+=		d_c99_decls_after_stmt.c
FILES+=		d_c99_decls_after_stmt2.c
FILES+=		d_c99_decls_after_stmt3.c
FILES+=		d_c99_flex_array_packed.c
FILES+=		d_c99_for_loops.c
FILES+=		d_c99_func.c
FILES+=		d_c99_nested_struct.c
FILES+=		d_c99_recursive_init.c
FILES+=		d_c99_struct_init.c
FILES+=		d_c99_union_cast.c
FILES+=		d_c99_union_cast.exp
FILES+=		d_c99_union_init1.c
FILES+=		d_c99_union_init2.c
FILES+=		d_c99_union_init3.c
FILES+=		d_c99_union_init4.c
FILES+=		d_c9x_array_init.c
FILES+=		d_c9x_recursive_init.c
FILES+=		d_cast_fun_array_param.c
FILES+=		d_cast_init.c
FILES+=		d_cast_init2.c
FILES+=		d_cast_lhs.c
FILES+=		d_cast_typeof.c
FILES+=		d_compound_literals1.c
FILES+=		d_compound_literals2.c
FILES+=		d_constant_conv1.c
FILES+=		d_constant_conv1.exp
FILES+=		d_constant_conv2.c
FILES+=		d_constant_conv2.exp
FILES+=		d_cvt_constant.c
FILES+=		d_cvt_constant.exp
FILES+=		d_cvt_in_ternary.c
FILES+=		d_decl_old_style_arguments.c
FILES+=		d_decl_old_style_arguments.exp
FILES+=		d_ellipsis_in_switch.c
FILES+=		d_fold_test.c
FILES+=		d_fold_test.exp
FILES+=		d_gcc_compound_statements1.c
FILES+=		d_gcc_compound_statements2.c
FILES+=		d_gcc_compound_statements3.c
FILES+=		d_gcc_extension.c
FILES+=		d_gcc_extension.exp
FILES+=		d_gcc_func.c
FILES+=		d_gcc_variable_array_init.c
FILES+=		d_incorrect_array_size.c
FILES+=		d_incorrect_array_size.exp
FILES+=		d_init_pop_member.c
FILES+=		d_init_pop_member.exp
FILES+=		d_lint_assert.c
FILES+=		d_lint_assert.exp
FILES+=		d_long_double_int.c
FILES+=		d_long_double_int.exp
FILES+=		d_nested_structs.c
FILES+=		d_nolimit_init.c
FILES+=		d_packed_structs.c
FILES+=		d_return_type.c
FILES+=		d_return_type.exp
FILES+=		d_shift_to_narrower_type.c
FILES+=		d_struct_init_nested.c
FILES+=		d_struct_init_nested.exp
FILES+=		d_type_conv1.c
FILES+=		d_type_conv1.exp
FILES+=		d_type_conv2.c
FILES+=		d_type_conv2.exp
FILES+=		d_type_conv3.c
FILES+=		d_type_conv3.exp
FILES+=		d_type_question_colon.c
FILES+=		d_typefun.c
FILES+=		d_typename_as_var.c
FILES+=		d_zero_sized_arrays.c
FILES+=		${:U0 ${:U:range=337}:C,^.$,0&,:C,^..$,0&,:@msg@msg_${msg}.c msg_${msg}.exp@}

# Note: only works for adding tests.
# To remove a test, the $$mi file must be edited manually.
sync-mi: .PHONY
	@set -eu;							\
	cd "${MAKEFILE:tA:H}/../../../..";				\
	mi="distrib/sets/lists/tests/mi";				\
	cvs update "$$mi";						\
	fmt="./usr/tests/usr.bin/xlint/lint1/%s\ttests-usr.bin-tests\tcompattestfile,atf\n"; \
	cat "$$mi" > "$$mi.tmp";					\
	printf "$$fmt" ${FILES:M*.c} ${FILES:M*.exp} >> "$$mi.tmp";	\
	distrib/sets/fmt-list "$$mi.tmp";				\
	mv "$$mi.tmp" "$$mi";						\
	cvs diff "$$mi" || true

.include <bsd.test.mk>
