/* $NetBSD: opt_bap.c,v 1.8 2023/05/20 12:05:01 rillig Exp $ */

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
