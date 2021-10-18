/* $NetBSD: opt_bap.c,v 1.3 2021/10/18 07:11:31 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-bap' and '-nbap'.
 *
 * The option '-bap' forces a blank line after every function body.
 *
 * The option '-nbap' keeps everything as is.
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
