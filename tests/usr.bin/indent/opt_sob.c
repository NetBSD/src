/* $NetBSD: opt_sob.c,v 1.8 2023/05/23 06:18:00 rillig Exp $ */

/*
 * Tests for the options '-sob' and '-nsob'.
 *
 * The option '-sob' swallows optional blank lines.
 *
 * XXX: The manual page says: "You can use this to get rid of blank lines
 * after declarations."; double check this.
 *
 * The option '-nsob' keeps optional blank lines as is.
 */

/*
 * FIXME: There are lots of 'optional blank lines' here that should be
 *  swallowed.
 */
//indent input
void		function_declaration(void);


int
function_with_0_blank_lines(void)
{
	int		var;
	var = value;
	if (var > 0)
		var--;
	if (var > 0) {
		var--;
	}
	return var;
}

int
function_with_1_blank_line(void)
{

	int		var;

	var = value;

	if (var > 0)
/* $ The following line is "optional" and is removed due to '-sob'. */

		var--;

	if (var > 0) {
/* $ The following line is "optional" and is removed due to '-sob'. */

		var--;

	}

	return var;

}


int
function_with_2_blank_lines(void)
{


	int		var;


	var = value;


	if (var > 0)
/* $ The following 2 lines are "optional" and are removed due to '-sob'. */


		var--;


	if (var > 0) {


		var--;


	}


	return var;


}
//indent end

//indent run -sob
void		function_declaration(void);


int
function_with_0_blank_lines(void)
{
	int		var;
	var = value;
	if (var > 0)
		var--;
	if (var > 0) {
		var--;
	}
	return var;
}

int
function_with_1_blank_line(void)
{

	int		var;

	var = value;

	if (var > 0)
		var--;

	if (var > 0) {
		var--;
// $ XXX: The following blank line may be considered optional.

	}

	return var;

}


int
function_with_2_blank_lines(void)
{

	int		var;

	var = value;

	if (var > 0)
		var--;

	if (var > 0) {
		var--;
// $ XXX: The following blank line may be considered optional.

	}

	return var;

}
//indent end

//indent run-equals-input -nsob
