/* $NetBSD: opt_bap.c,v 1.9 2023/05/23 06:18:00 rillig Exp $ */

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


//indent input
#if 0
void
f(void)
{
}
#else
#endif
//indent end

//indent run -bacc -bap
#if 0
void
f(void)
{
}
// $ The following blank line may be considered optional, as it precedes a
// $ preprocessing line.  In that case, the -bap option would only apply to
// $ elements on the same syntactic level, such as function definitions and
// $ other declarations.

#else
#endif
//indent end
