/* $NetBSD: lsym_lparen_or_lbracket.c,v 1.6 2022/04/24 08:48:17 rillig Exp $ */

/*
 * Tests for the token lsym_lparen_or_lbracket, which represents a '(' or '['
 * in these contexts:
 *
 * In a type name, '(' constructs a function type.
 *
 * In an expression, '(' starts an inner expression to override the usual
 * operator precedence.
 *
 * In a function call expression, '(' marks the beginning of the function
 * arguments.
 *
 * In a 'sizeof' expression, '(' is required if the argument is a type name.
 *
 * In an expression, '(' followed by a type name starts a cast expression or
 * a compound literal.
 *
 * In a type declaration, '(' marks the beginning of the function parameters.
 *
 * After one of the keywords 'for', 'if', 'switch' or 'while', the controlling
 * expression must be enclosed in '(' and ')'; see lsym_for.c, lsym_if.c,
 * lsym_switch.c, lsym_while.c.
 *
 * In a declaration, '[' derives an array type.
 *
 * In an expression, '[' starts an array subscript.
 */

/* The '(' in a type name derives a function type. */
#indent input
typedef void signal_handler(int);
void (*signal(void (*)(int)))(int);
#indent end

#indent run
typedef void signal_handler(int);
void		(*signal(void (*)(int)))(int);
#indent end


#indent input
#define macro(arg) ((arg) + 1)
#indent end

#indent run-equals-input -di0


/*
 * The '(' in an expression overrides operator precedence.  In multi-line
 * expressions, the continuation lines are aligned on the parentheses.
 */
#indent input
int nested = (
	(
		(
			(
				1 + 4
			)
		)
	)
);
#indent end

#indent run
int		nested = (
			  (
			   (
			    (
			     1 + 4
			     )
			    )
			   )
);
#indent end


/* The '(' in a function call expression starts the argument list. */
#indent input
int var = macro_call ( arg1,  arg2  ,arg3);
#indent end

#indent run
int		var = macro_call(arg1, arg2, arg3);
#indent end


/*
 * The '(' in a sizeof expression is required for type names and optional for
 * expressions.
 */
#indent input
size_t sizeof_typename = sizeof ( int );
size_t sizeof_expr = sizeof ( 12345 ) ;
#indent end

#indent run
size_t		sizeof_typename = sizeof(int);
size_t		sizeof_expr = sizeof(12345);
#indent end


/* The '[' in a type name derives an array type. */
#indent input
int array_of_numbers[100];
#indent end

#indent run
int		array_of_numbers[100];
#indent end


/* The '[' in an expression accesses an array element. */
#indent input
int second_prime = &primes[1];
#indent end

#indent run
int		second_prime = &primes[1];
#indent end


#indent input
void
function(void)
{
	/* Type casts */
	a = (int)b;
	a = (struct tag)b;
	/* TODO: The '(int)' is not a type cast, it is a prototype list. */
	a = (int (*)(int))fn;

	/* Not type casts */
	a = sizeof(int) * 2;
	a = sizeof(5) * 2;
	a = offsetof(struct stat, st_mtime);

	/* Grouping subexpressions */
	a = ((((b + c)))) * d;
}
#indent end

#indent run-equals-input


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

/* See t_errors.sh, test case 'compound_literal'. */
