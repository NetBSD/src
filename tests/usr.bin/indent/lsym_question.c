/* $NetBSD: lsym_question.c,v 1.5 2023/05/15 10:13:40 rillig Exp $ */

/*
 * Tests for the token lsym_question, which represents the '?' in a '?:'
 * conditional expression.
 */

//indent input
const char *result = cond ? "then" : "else";

const char *multi = cond1 ? "cond1" : cond2 ? "cond2" : cond3 ? "cond3" : "";
//indent end

//indent run-equals-input -di0


/*
 * To make them easier to read, conditional expressions can be split into
 * multiple lines.
 */
//indent input
const char *separate_lines = cond
	? "then"
	: "else";
//indent end

//indent run -di0
const char *separate_lines = cond
// $ XXX: Continuation lines in expressions should be indented, even in column 1.
? "then"
: "else";
//indent end


/*
 * In functions, conditional expressions are indented as intended.
 */
//indent input
void
function(void)
{
	return cond
		? "then"
		: "else";
}
//indent end

//indent run-equals-input


/*
 * In functions, conditional expressions are indented as intended.
 */
//indent input
void
function(void)
{
	const char *branch = cond
		? "then"
		: "else";

	const char *multiple_branches = cond1
		? "then 1"
		: cond2
		? "then 2"
		: "else";

	const char *condensed = cond1 ? "condensed 1"
		: cond2 ? "condensed 2"
		: "condensed else";
}
//indent end

//indent run-equals-input -di0
