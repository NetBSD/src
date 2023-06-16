/* $NetBSD: opt_bap.c,v 1.10 2023/06/16 11:48:32 rillig Exp $ */

/*
 * Tests for the options '-bap' and '-nbap' ("blank line after procedure
 * body").
 *
 * The option '-bap' forces a blank line after every function body.
 *
 * The option '-nbap' keeps everything as is.
 */

//indent input
static void one_liner(void){}
static void several_lines(void)
{
	action();
}
int main(void){}
int global_variable;

void already_has_blank_line_below(void)
{
}

void has_several_blank_lines_below(void)
{
}



int		the_end;
//indent end

//indent run -bap
static void
one_liner(void)
{
}

static void
several_lines(void)
{
	action();
}

int
main(void)
{
}

int		global_variable;

void
already_has_blank_line_below(void)
{
}

void
has_several_blank_lines_below(void)
{
}



int		the_end;
//indent end

//indent run -nbap
static void
one_liner(void)
{
}
static void
several_lines(void)
{
	action();
}
int
main(void)
{
}
int		global_variable;

void
already_has_blank_line_below(void)
{
}

void
has_several_blank_lines_below(void)
{
}



int		the_end;
//indent end


/*
 * Don't insert a blank line between the end of a function body and an '#endif'
 * line, as both are closing elements.
 */
//indent input
#if 0
void
example(void)
{
}
#endif
//indent end

//indent run-equals-input -bap


/*
 * A preprocessing line after the end of a function body does not force a blank
 * line, as these lines are not a different syntactic layer.
 */
//indent input
#if 0
void
f(void)
{
}
#else
#endif
//indent end

//indent run-equals-input -bacc -bap


/*
 * Do not add a blank line between the end of a function body and an '#undef',
 * as this is a common way to undefine a function-local macro.
 */
//indent input
#define replace
{
}
#undef replace
//indent end

//indent run-equals-input -bap

//indent run-equals-input -bap -bacc
