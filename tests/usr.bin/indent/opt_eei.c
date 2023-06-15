/* $NetBSD: opt_eei.c,v 1.15 2023/06/15 09:19:07 rillig Exp $ */

/*
 * Tests for the options '-eei' and '-neei'.
 *
 * The option '-eei' enables extra indentation on continuation lines of the
 * expression part of 'if' and 'while' statements. These continuation lines
 * are indented one extra level to avoid being confused for the first
 * statement of the body, even if the condition line starts with an operator
 * such as '&&' or '<' that could not start a statement.
 *
 * The option '-neei' indents these conditions in the same way as all other
 * continued statements.
 */

//indent input
{
	if (a <
b)
		stmt();
	if (a
<
b)
		stmt();
	while (a
< b)
		stmt();
	switch (
		a)
		stmt();
}
//indent end

/*
 * By default, continuation lines are aligned on parentheses, and only a
 * multi-line switch statement would have ambiguous indentation.
 */
//indent run
{
	if (a <
	    b)
		stmt();
	if (a
	    <
	    b)
		stmt();
	while (a
	       < b)
		stmt();
	switch (
		a)
		stmt();
}
//indent end

//indent run-equals-prev-output -neei

/*
 * For indentation 8, the only expression that needs to be disambiguated is
 * the one from the switch statement.
 */
//indent run -eei
{
	if (a <
	    b)
		stmt();
	if (a
	    <
	    b)
		stmt();
	while (a
	       < b)
		stmt();
	switch (
			a)
		stmt();
}
//indent end

/* For indentation 4, the expressions from the 'if' are ambiguous. */
//indent run -neei -i4
{
    if (a <
	b)
	stmt();
    if (a
	<
	b)
	stmt();
    while (a
	   < b)
	stmt();
    switch (
	    a)
	stmt();
}
//indent end

//indent run -eei -i4
{
    if (a <
	    b)
	stmt();
    if (a
	    <
	    b)
	stmt();
    while (a
	   < b)
	stmt();
    switch (
/* $ XXX: No extra indentation necessary. */
	    a)
	stmt();
}
//indent end

/*
 * The -nlp option uses a fixed indentation for continuation lines. The if
 * statements are disambiguated.
 */
//indent run -eei -i4 -nlp
{
    if (a <
	    b)
	stmt();
    if (a
	    <
	    b)
	stmt();
    while (a
	    < b)
	stmt();
    switch (
	    a)
	stmt();
}
//indent end

/* With a continuation indentation of 2, there is no ambiguity at all. */
//indent run -eei -i6 -ci2 -nlp
{
      if (a <
	b)
	    stmt();
      if (a
	<
	b)
	    stmt();
      while (a
	< b)
	    stmt();
      switch (
	a)
	    stmt();
}
//indent end


/*
 * Ensure that after a condition with extra indentation, the following
 * statements are not affected.
 */
//indent input
{
	if (
		cond
	)
		stmt(
			arg
		);
}
//indent end

//indent run -eei -nlp -i4
{
    if (
	    cond
	)
	stmt(
	    arg
	    );
}
//indent end


/*
 * When multi-line expressions are aligned on the parentheses, they may have an
 * ambiguous indentation as well.
 */
//indent input
{
	if (fun(
		1,
		2,
		3))
		stmt;
}
//indent end

//indent run-equals-input

//indent run -eei
{
	if (fun(
			1,
			2,
			3))
		stmt;
}
//indent end


//indent input
{
	if (((
		3
	)))
		stmt;
	if ((((
		4
	))))
		stmt;
}
//indent end

//indent run -ci2 -nlp -eei
{
	if (((
	      3
	    )))
		stmt;
	if ((((
		  4
	      ))))
		stmt;
}
//indent end
