/* $NetBSD: opt_bad.c,v 1.12 2023/06/26 14:54:40 rillig Exp $ */

/*
 * Tests for the options '-bad' and '-nbad'.
 *
 * The option '-bad' forces a blank line after every block of declarations.
 * It only affects declarations of local variables.  It does not affect
 * file-scoped declarations or definitions.
 *
 * The option '-nbad' leaves everything as is.
 */

/* Test global declarations. */
//indent input
int		global_variable;
void		function_declaration(void);
#if 0
#endif
/* comment */
//indent end

//indent run-equals-input -bad

//indent run-equals-input -nbad


/* See FreeBSD r303599. */
//indent input
#if defined(__i386__)
int a;
#elif defined(__amd64__)
int b;
#else
#error "Port me"
#endif
//indent end

//indent run -bad
#if defined(__i386__)
int		a;
#elif defined(__amd64__)
int		b;
#else
#error "Port me"
#endif
//indent end


/* Test local declarations. */
//indent input
void function_definition(void) {
    int local_variable;
    function_call();
    int local_variable_after_statement;
    function_call();
}
//indent end

//indent run -bad
void
function_definition(void)
{
	int		local_variable;

	function_call();
	int		local_variable_after_statement;

	function_call();
}
//indent end

//indent run -nbad
void
function_definition(void)
{
	int		local_variable;
	/* $ No blank line here. */
	function_call();
	int		local_variable_after_statement;
	/* $ No blank line here. */
	function_call();
}
//indent end


/*
 * A comment after a declaration does not change whether there should be a
 * blank line below the declaration.
 */
//indent input
void
comments(void)
{
	int local_var_1;	/* comment */
	int local_var_2;	/* comment */
	/* comment line */
	function_call();
}
//indent end

//indent run -ldi0 -bad
void
comments(void)
{
	int local_var_1;	/* comment */
	int local_var_2;	/* comment */
// $ Indent does not look ahead much, so it doesn't know whether this comment
// $ will be followed by a declaration or a statement.
	/* comment line */

	function_call();
}
//indent end

//indent run-equals-input -ldi0 -nbad


/*
 * A declaration that has a braced initializer is still a declaration and
 * therefore needs a blank line below.
 */
//indent input
void
initializer(void)
{
	int local_var_init_1[] = {1};
	int local_var_init_2[] = {1};
	function_call();
}

void
initializer_with_blank(void)
{
	int local_var_init_1[] = {1};

	int local_var_init_2[] = {1};

	function_call();
}
//indent end

//indent run -ldi0 -bad
void
initializer(void)
{
	int local_var_init_1[] = {1};
	int local_var_init_2[] = {1};

	function_call();
}

void
initializer_with_blank(void)
{
	int local_var_init_1[] = {1};

	int local_var_init_2[] = {1};

	function_call();
}
//indent end

//indent run-equals-input -ldi0 -nbad


//indent input
{
	// $ The '}' in an initializer does not finish a declaration,
	// $ only a semicolon does.
	int decl1[2][2] = {
		{1, 2},
		{3, 4},
	};
	/* comment */
	int decl2;
	// $ If the declaration is followed by a '}' that terminates the block
	// $ statement, * there is no need for a blank line before the '}'.
}
//indent end

//indent run-equals-input -bad -di0
