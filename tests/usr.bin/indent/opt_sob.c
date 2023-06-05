/* $NetBSD: opt_sob.c,v 1.10 2023/06/05 12:01:34 rillig Exp $ */

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

	}

	return var;

}
//indent end

//indent run-equals-input -nsob


//indent input
{
	switch (expr) {

	case 1:

	}
}
//indent end

//indent run-equals-input -sob

//indent run -sob -bl
{
	switch (expr)
	{

	case 1:

	}
}
//indent end
