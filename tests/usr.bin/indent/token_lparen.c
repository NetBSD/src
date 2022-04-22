/* $NetBSD: token_lparen.c,v 1.10 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the tokens '(', which has several possible meanings, and for '['.
 *
 * In an expression, '(' overrides the precedence rules by explicitly grouping
 * a subexpression in parentheses.
 *
 * In an expression, '(' marks the beginning of a type cast or conversion.
 *
 * In a function call expression, '(' marks the beginning of the function
 * arguments.
 *
 * In a type declaration, '(' marks the beginning of the function parameters.
 */

/* This is the maximum supported number of parentheses. */
#indent input
int zero = (((((((((((((((((((0)))))))))))))))))));
#indent end

#indent run-equals-input -di0


#indent input
void (*action)(void);
#indent end

#indent run-equals-input -di0


#indent input
#define macro(arg) ((arg) + 1)
#indent end
#indent run-equals-input -di0


#indent input
void
function(void)
{
    other_function();
    other_function("first", 2, "last argument"[4]);

    if (false)(void)x;
    if (false)(func)(arg);
    if (false)(cond)?123:456;

    /* C99 compound literal */
    origin = (struct point){0,0};

    /* GCC statement expression */
    /* expr = ({if(expr)debug();expr;}); */
/* $ XXX: Generates 'error: Standard Input:36: Unbalanced parentheses'. */
}
#indent end

#indent run
void
function(void)
{
	other_function();
	other_function("first", 2, "last argument"[4]);

	if (false)
		(void)x;
	if (false)
		(func)(arg);
	if (false)
		(cond) ? 123 : 456;

	/* C99 compound literal */
	origin = (struct point){
		0, 0
	};

	/* GCC statement expression */
	/* expr = ({if(expr)debug();expr;}); */
}
#indent end


/*
 * C99 designator initializers are the rare situation where there is a space
 * before a '['.
 */
#indent input
int array[] = {
	1, 2, [2] = 3, [3] = 4,
};
#indent end

#indent run-equals-input -di0


/*
 * Test want_blank_before_lparen for all possible token types.
 */
#indent input
void cover_want_blank_before_lparen(void)
{
	/* ps.prev_token can never be 'newline'. */
	int newline =
	(3);

	int lparen_or_lbracket = a[(3)];
	int rparen_or_rbracket = a[3](5);
	+(unary_op);
	3 + (binary_op);
	a++(postfix_op);	/* unlikely to be seen in practice */
	cond ? (question) : (5);
	switch (expr) {
	case (case_label):;
	}
	a ? 3 : (colon);
	(semicolon) = 3;
	int lbrace[] = {(3)};
	int rbrace_in_decl = {{3}(4)};	/* syntax error */
	{}
	(rbrace_in_stmt)();
	ident(3);
	int(decl);
	a++, (comma)();
	int comment = /* comment */ (3);	/* comment is skipped */
	switch (expr) {}
#define preprocessing
	(preprocessing)();
	/* $ XXX: lsym_form_feed should be skipped, just as newline. */
	(lsym_form_feed)();	/* XXX: should be skipped */
	for(;;);
	do(lsym_do)=3;while(0);
	if(cond);else(lsym_else)();
	do(lsym_do);while(0);
	str.(member);		/* syntax error */
	L("string_prefix");		/* impossible */
	static (int)storage_class;	/* syntax error */
	funcname(3);
	typedef (type_def) new_type;
	// $ TODO: is keyword_struct_union_enum possible?
	struct (keyword_struct_union_enum);	/* syntax error */
}
#indent end

#indent run -ldi0
void
cover_want_blank_before_lparen(void)
{
	/* ps.prev_token can never be 'newline'. */
	int newline =
	(3);

	int lparen_or_lbracket = a[(3)];
	int rparen_or_rbracket = a[3](5);
	+(unary_op);
	3 + (binary_op);
	a++ (postfix_op);	/* unlikely to be seen in practice */
	cond ? (question) : (5);
	switch (expr) {
	case (case_label):;
	}
	a ? 3 : (colon);
	(semicolon) = 3;
	int lbrace[] = {(3)};
	int rbrace_in_decl = {{3} (4)};	/* syntax error */
	{
	}
	(rbrace_in_stmt)();
	ident(3);
	int (decl);
	a++, (comma)();
	int comment = /* comment */ (3);	/* comment is skipped */
	switch (expr) {
	}
#define preprocessing
	(preprocessing)();

/* $ XXX: Where has the '\f' gone? It should have been preserved. */
	(lsym_form_feed)();	/* XXX: should be skipped */
	for (;;);
	do
		(lsym_do) = 3;
	while (0);
	if (cond);
	else
		(lsym_else)();
	do
		(lsym_do);
	while (0);
	str.(member);		/* syntax error */
	L("string_prefix");	/* impossible */
	static (int)storage_class;	/* syntax error */
	funcname(3);
	typedef (type_def) new_type;
	struct (keyword_struct_union_enum);	/* syntax error */
}
#indent end
