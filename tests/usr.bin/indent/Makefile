#	$NetBSD: Makefile,v 1.24 2021/10/19 20:20:24 rillig Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/indent
TESTS_SH=	t_errors
TESTS_SH+=	t_indent
TESTS_SH+=	t_misc
TESTS_SH+=	t_options

FILESDIR=	${TESTSDIR}
FILES=		block.0
FILES+=		block.0.stdout
FILES+=		declarations.0
FILES+=		declarations.0.stderr
FILES+=		declarations.0.stdout
FILES+=		elsecomment.0
FILES+=		elsecomment.0.stdout
FILES+=		elsecomment.0.pro
FILES+=		f_decls.0
FILES+=		f_decls.0.stdout
FILES+=		indent_off_on.c
FILES+=		indent_variables.0
FILES+=		indent_variables.0.pro
FILES+=		indent_variables.0.stdout
FILES+=		label.c
FILES+=		lex.0
FILES+=		lex.0.pro
FILES+=		lex.0.stdout
FILES+=		lex_char.0
FILES+=		lex_char.0.pro
FILES+=		lex_char.0.stdout
FILES+=		lex_string.0
FILES+=		lex_string.0.pro
FILES+=		lex_string.0.stdout
FILES+=		lineno.0
FILES+=		lineno.0.pro
FILES+=		lineno.0.stdout
FILES+=		list_head.0
FILES+=		list_head.0.stdout
FILES+=		offsetof.0
FILES+=		offsetof.0.stdout
FILES+=		opt--version.0
FILES+=		opt--version.0.pro
FILES+=		opt--version.0.stdout
FILES+=		opt-P.0
FILES+=		opt-P.0.pro
FILES+=		opt-P.0.stdout
FILES+=		opt-T.0
FILES+=		opt-T.0.pro
FILES+=		opt-T.0.stdout
FILES+=		opt-U.0
FILES+=		opt-U.0.list
FILES+=		opt-U.0.pro
FILES+=		opt-U.0.stdout
FILES+=		opt-bap+sob.0
FILES+=		opt-bap+sob.0.pro
FILES+=		opt-bap+sob.0.stdout
FILES+=		opt-bl.0
FILES+=		opt-bl.0.pro
FILES+=		opt-bl.0.stdout
FILES+=		opt-br.0
FILES+=		opt-br.0.pro
FILES+=		opt-br.0.stdout
FILES+=		opt-c.0
FILES+=		opt-c.0.pro
FILES+=		opt-c.0.stdout
FILES+=		opt-cd.0
FILES+=		opt-cd.0.pro
FILES+=		opt-cd.0.stdout
FILES+=		opt-ci.0
FILES+=		opt-ci.0.pro
FILES+=		opt-ci.0.stdout
FILES+=		opt-cli.0
FILES+=		opt-cli.0.pro
FILES+=		opt-cli.0.stdout
FILES+=		opt-d.0
FILES+=		opt-d.0.pro
FILES+=		opt-d.0.stdout
FILES+=		opt-di.0
FILES+=		opt-di.0.pro
FILES+=		opt-di.0.stdout
FILES+=		opt-i.0
FILES+=		opt-i.0.pro
FILES+=		opt-i.0.stdout
FILES+=		opt-l.0
FILES+=		opt-l.0.pro
FILES+=		opt-l.0.stdout
FILES+=		opt-lc.0
FILES+=		opt-lc.0.pro
FILES+=		opt-lc.0.stdout
FILES+=		opt-ldi.0
FILES+=		opt-ldi.0.pro
FILES+=		opt-ldi.0.stdout
FILES+=		opt-npro.0
FILES+=		opt-npro.0.pro
FILES+=		opt-npro.0.stdout
FILES+=		opt-ta.0
FILES+=		opt-ta.0.pro
FILES+=		opt-ta.0.stdout
FILES+=		opt-ts.0
FILES+=		opt-ts.0.pro
FILES+=		opt-ts.0.stdout
FILES+=		opt.0
FILES+=		opt.0.pro
FILES+=		opt.0.stdout
FILES+=		opt_bacc.c
FILES+=		opt_bad.c
FILES+=		opt_badp.c
FILES+=		opt_bap.c
FILES+=		opt_bbb.c
FILES+=		opt_bc.c
FILES+=		opt_bs.c
FILES+=		opt_cdb.c
FILES+=		opt_ce.c
FILES+=		opt_cs.c
FILES+=		opt_dj.c
FILES+=		opt_eei.c
FILES+=		opt_ei.c
FILES+=		opt_fbs.c
FILES+=		opt_fc1.c
FILES+=		opt_fcb.c
FILES+=		opt_ip.c
FILES+=		opt_lp.c
FILES+=		opt_lpl.c
FILES+=		opt_pcs.c
FILES+=		opt_psl.c
FILES+=		opt_sc.c
FILES+=		opt_sob.c
FILES+=		opt_ut.c
FILES+=		opt_v.c
FILES+=		parens.0
FILES+=		parens.0.stdout
FILES+=		parens.0.pro
FILES+=		ps_ind_level.c
FILES+=		struct.0
FILES+=		struct.0.stdout
FILES+=		surplusbad.0
FILES+=		surplusbad.0.stdout
FILES+=		surplusbad.0.pro
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
FILES+=		types_from_file.0
FILES+=		types_from_file.0.stdout
FILES+=		types_from_file.0.list
FILES+=		types_from_file.0.pro
FILES+=		wchar.0
FILES+=		wchar.0.stdout

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
