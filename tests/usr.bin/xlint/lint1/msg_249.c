/*	$NetBSD: msg_249.c,v 1.9 2022/01/15 23:21:34 rillig Exp $	*/
# 3 "msg_249.c"

// Test for message: syntax error '%s' [249]

/*
 * Cover the grammar rule 'top_level_declaration: error T_SEMI'.
 */
/* expect+1: syntax error '"' [249] */
"syntax error in top_level_declaration";

/* XXX: This is necessary to recover the yacc parser. */
int recover_from_semi;

/*
 * Cover the grammar rule 'top_level_declaration: error T_RBRACE'.
 */
/* expect+1: syntax error '"' [249] */
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
	/* expect+2: warning: statement not reached [193] */
	if (0)
		;
	);			/* expect: syntax error ')' */
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
