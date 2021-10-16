/* $NetBSD: opt_sob.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

#indent input
/* $ FIXME: There are lots of 'optional blank lines' here that should */
/* $ FIXME: be swallowed. */
void		function_declaration(void);


int
function_with_0_blank_lines(void)
{
	int		var;
	var = value;
	if (var > 0)
		var--;
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


	return var;


}
#indent end

#indent run -sob
void		function_declaration(void);


int
function_with_0_blank_lines(void)
{
	int		var;
	var = value;
	if (var > 0)
		var--;
	return var;
}

int
function_with_1_blank_line(void)
{

	int		var;

	var = value;

	if (var > 0)
		var--;

	return var;

}


int
function_with_2_blank_lines(void)
{


	int		var;


	var = value;


	if (var > 0)
		var--;


	return var;


}
#indent end

#indent run -nsob
void		function_declaration(void);


int
function_with_0_blank_lines(void)
{
	int		var;
	var = value;
	if (var > 0)
		var--;
	return var;
}

int
function_with_1_blank_line(void)
{

	int		var;

	var = value;

	if (var > 0)

		var--;

	return var;

}


int
function_with_2_blank_lines(void)
{


	int		var;


	var = value;


	if (var > 0)


		var--;


	return var;


}
#indent end
