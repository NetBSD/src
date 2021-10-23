#	$NetBSD: Makefile,v 1.33 2021/10/23 21:45:14 rillig Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/indent
TESTS_SH=	t_errors
TESTS_SH+=	t_misc
TESTS_SH+=	t_options

FILESDIR=	${TESTSDIR}
FILES=		fmt_block.c
FILES+=		fmt_decl.c
FILES+=		fmt_else_comment.c
FILES+=		fmt_expr.c
FILES+=		indent_off_on.c
FILES+=		label.c
FILES+=		lex_ident.c
FILES+=		lex_char.c
FILES+=		lex_string.c
FILES+=		opt_P.c
FILES+=		opt_T.c
FILES+=		opt_bacc.c
FILES+=		opt_bad.c
FILES+=		opt_badp.c
FILES+=		opt_bap.c
FILES+=		opt_bap_sob.c
FILES+=		opt_bbb.c
FILES+=		opt_bc.c
FILES+=		opt_bl_br.c
FILES+=		opt_bs.c
FILES+=		opt_c.c
FILES+=		opt_cd.c
FILES+=		opt_cdb.c
FILES+=		opt_ce.c
FILES+=		opt_ci.c
FILES+=		opt_cli.c
FILES+=		opt_cs.c
FILES+=		opt_d.c
FILES+=		opt_di.c
FILES+=		opt_dj.c
FILES+=		opt_eei.c
FILES+=		opt_ei.c
FILES+=		opt_fbs.c
FILES+=		opt_fc1.c
FILES+=		opt_fcb.c
FILES+=		opt_i.c
FILES+=		opt_ip.c
FILES+=		opt_l.c
FILES+=		opt_lc.c
FILES+=		opt_ldi.c
FILES+=		opt_lp.c
FILES+=		opt_lpl.c
FILES+=		opt_pcs.c
FILES+=		opt_psl.c
FILES+=		opt_sc.c
FILES+=		opt_sob.c
FILES+=		opt_ta.c
FILES+=		opt_ts.c
FILES+=		opt_ut.c
FILES+=		opt_v.c
FILES+=		opt_version.c
FILES+=		ps_ind_level.c
FILES+=		t_options.awk
FILES+=		token_binary_op.c
FILES+=		token_case_label.c
FILES+=		token_colon.c
FILES+=		token_comma.c
FILES+=		token_comment.c
FILES+=		token_decl.c
FILES+=		token_do_stmt.c
FILES+=		token_end_of_file.c
FILES+=		token_for_exprs.c
FILES+=		token_form_feed.c
FILES+=		token_funcname.c
FILES+=		token_ident.c
FILES+=		token_if_expr.c
FILES+=		token_if_expr_stmt.c
FILES+=		token_if_expr_stmt_else.c
FILES+=		token_keyword_do.c
FILES+=		token_keyword_do_else.c
FILES+=		token_keyword_else.c
FILES+=		token_keyword_for_if_while.c
FILES+=		token_keyword_struct_union_enum.c
FILES+=		token_lbrace.c
FILES+=		token_lparen.c
FILES+=		token_newline.c
FILES+=		token_period.c
FILES+=		token_postfix_op.c
FILES+=		token_preprocessing.c
FILES+=		token_question.c
FILES+=		token_rbrace.c
FILES+=		token_rparen.c
FILES+=		token_semicolon.c
FILES+=		token_stmt.c
FILES+=		token_stmt_list.c
FILES+=		token_storage_class.c
FILES+=		token_string_prefix.c
FILES+=		token_switch_expr.c
FILES+=		token_type_def.c
FILES+=		token_unary_op.c
FILES+=		token_while_expr.c

add-test: .PHONY
	@set -eu; \
	test=${NAME:Q}.c; \
	if [ "$$test" = ".c" ]; then \
		echo "usage: ${MAKE} add-test NAME=<name>"; \
		exit; \
	fi; \
	\
	if [ -f "$$test" ]; then \
		echo "error: test $$test already exists." 1>&2; \
		exit 1; \
	fi; \
	\
	echo "=> Adding test $$test"; \
	printf '%s\n' \
		'/* $$''NetBSD$$ */' \
		'/* $$''FreeBSD$$ */' \
		'' \
		'/*' \
		' * TODO: Explain the purpose of the test.' \
		' */' \
		'' \
		'#indent input' \
		'// TODO: add input' \
		'#indent end' \
		'' \
		'#indent run [-options]' \
		'// TODO: add expected output' \
		'#indent end' \
	> "$$test"; \
	cvs add "$$test"; \
	printf '%s\n' \
		'/^FILES+=/i' \
		"FILES+=		$$test" \
		'.' 'w' 'q' \
	| ed Makefile; \
	${MAKE} sync-mi

# Note: only works for adding tests.
# To remove a test, the $$mi file must be edited manually.
sync-mi: .PHONY
	@set -eu;							\
	cd "${MAKEFILE:tA:H}/../../..";					\
	mi="distrib/sets/lists/tests/mi";				\
	cvs update "$$mi";						\
	fmt="./usr/tests/usr.bin/indent/%s\ttests-usr.bin-tests\tcompattestfile,atf\n"; \
	cat "$$mi" > "$$mi.tmp";					\
	printf "$$fmt" ${FILES:M${NAME}*} >> "$$mi.tmp";		\
	distrib/sets/fmt-list "$$mi.tmp";				\
	mv "$$mi.tmp" "$$mi";						\
	cvs diff "$$mi" || true

.include <bsd.test.mk>
