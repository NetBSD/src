/* $NetBSD: opt_pcs.c,v 1.13 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the options '-pcs' and '-npcs'.
 *
 * The option '-pcs' adds a space in a function call expression, between the
 * function name and the opening parenthesis.
 *
 * The option '-npcs' removes any whitespace from a function call expression,
 * between the function name and the opening parenthesis.
 */

//indent input
void
example(void)
{
	function_call();
	function_call (1);
	function_call   (1,2,3);
}
//indent end

//indent run -pcs
void
example(void)
{
	function_call ();
	function_call (1);
	function_call (1, 2, 3);
}
//indent end

//indent run -npcs
void
example(void)
{
	function_call();
	function_call(1);
	function_call(1, 2, 3);
}
//indent end


//indent input
void ( * signal ( void ( * handler ) ( int ) ) ) ( int ) ;
int var = (function)(arg);
//indent end

//indent run -di0 -pcs
void (*signal (void (*handler) (int))) (int);
int var = (function) (arg);
//indent end

//indent run -di0 -npcs
void (*signal(void (*handler)(int)))(int);
int var = (function)(arg);
//indent end


/*
 * The option '-pcs' also applies to 'sizeof' and 'offsetof', even though
 * these are not functions.
 */
//indent input
int sizeof_type = sizeof   (int);
int sizeof_type = sizeof(int);
int sizeof_expr = sizeof   (0);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof   0;

int offset = offsetof(struct s, member);
int offset = offsetof   (struct s, member);
//indent end

/* The option '-pcs' overrides '-nbs'. */
//indent run -pcs -di0 -nbs
int sizeof_type = sizeof (int);
int sizeof_type = sizeof (int);
int sizeof_expr = sizeof (0);
int sizeof_expr = sizeof (0);
int sizeof_expr = sizeof 0;

int offset = offsetof (struct s, member);
int offset = offsetof (struct s, member);
//indent end

/*
 * If the option '-npcs' is given, '-bs' can still specialize it. This only
 * applies to 'sizeof', but not 'offsetof'.
 */
//indent run -npcs -di0 -bs
int sizeof_type = sizeof (int);
int sizeof_type = sizeof (int);
int sizeof_expr = sizeof (0);
int sizeof_expr = sizeof (0);
int sizeof_expr = sizeof 0;

int offset = offsetof(struct s, member);
int offset = offsetof(struct s, member);
//indent end

//indent run -npcs -di0
int sizeof_type = sizeof(int);
int sizeof_type = sizeof(int);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof(0);
int sizeof_expr = sizeof 0;

int offset = offsetof(struct s, member);
int offset = offsetof(struct s, member);
//indent end
