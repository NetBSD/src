/* $NetBSD: opt_bap.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

#indent input
static void one_liner(void) { /* FIXME: needs empty line below */ }
static void several_lines(void) {
	action();
	/* FIXME: needs empty line below */
}
int main(void){/* FIXME: needs empty line below */}
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
{				/* FIXME: needs empty line below */
}
static void
several_lines(void)
{
	action();
	/* FIXME: needs empty line below */
}
int
main(void)
{				/* FIXME: needs empty line below */
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
#indent end

#indent input
static void one_liner(void) {}
static void several_lines(void) {
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

#indent run -nbap
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
#indent end
