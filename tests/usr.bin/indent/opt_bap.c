/* $NetBSD: opt_bap.c,v 1.5 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the options '-bap' and '-nbap' ("blank line after procedure
 * body").
 *
 * The option '-bap' forces a blank line after every function body.
 *
 * The option '-nbap' keeps everything as is.
 *
 * FIXME: These options don't have any effect since at least 2000.
 * TODO: Investigate how nobody could have noticed this for 20 years.
 */

#indent input
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
#indent end

#indent run -bap
static void
one_liner(void)
{
}
/* $ FIXME: needs a blank line here */
static void
several_lines(void)
{
	action();
}
/* $ FIXME: needs a blank line here */
int
main(void)
{
}
/* $ FIXME: needs a blank line here */
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
#indent end

#indent run-equals-prev-output -nbap
