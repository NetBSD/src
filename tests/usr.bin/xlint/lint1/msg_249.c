/*	$NetBSD: msg_249.c,v 1.15 2024/01/28 08:17:27 rillig Exp $	*/
# 3 "msg_249.c"

// Test for message: syntax error '%s' [249]

/* lint1-extra-flags: -X 351 */

/*
 * Cover the grammar rule 'top_level_declaration: error T_SEMI'.
 */
/* expect+1: error: syntax error '"' [249] */
"syntax error in top_level_declaration";

/* XXX: This is necessary to recover the yacc parser. */
int recover_from_semi;

/*
 * Cover the grammar rule 'top_level_declaration: error T_RBRACE'.
 */
/* expect+1: error: syntax error '"' [249] */
"syntax error in top_level_declaration"}

/* XXX: This is necessary to recover the yacc parser. */
int recover_from_rbrace;

/*
 * Before func.c 1.110 from 2021-06-19, lint ran into this:
 * assertion "cstmt->c_kind == kind" failed in end_control_statement
 */
void
function(void)
{
	if (0)
		/* expect+1: warning: statement not reached [193] */
		;
	/* expect+1: error: syntax error ')' [249] */
	);
}

/* XXX: It is unexpected that this error is not detected. */
"This syntax error is not detected.";

/* XXX: This is necessary to recover the yacc parser. */
double recover_from_rparen;

/* Ensure that the declaration after the syntax error is processed. */
double *
access_declaration_after_syntax_error(void)
{
	return &recover_from_rparen;
}

struct cover_member_declaration {
	/* cover 'noclass_declmods ... notype_member_decls' */
	const noclass_declmods;

	/* cover 'noclass_declspecs ...' */
	const int noclass_declspecs;

	/* cover 'add_type_qualifier_list end_type' */
	/* expect+1: error: syntax error 'member without type' [249] */
	const;
};

/*
 * At this point, lint assumes that the following code is still in the
 * function 'access_declaration_after_syntax_error'.
 */

int gcc_statement_expression_1 = ({
unused_label:
	1;
	1;
	/* expect+2: error: syntax error 'labels are only valid inside a function' [249] */
	/* expect+1: error: non-constant initializer [177] */
});

/* Even another function definition does not help. */
void
try_to_recover(void)
{
}

int gcc_statement_expression_2 = ({
unused_label:
	1;
	1;
	/* expect+2: error: syntax error 'labels are only valid inside a function' [249] */
	/* expect+1: error: non-constant initializer [177] */
});
