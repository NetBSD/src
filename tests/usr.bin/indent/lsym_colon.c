/* $NetBSD: lsym_colon.c,v 1.5 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the token lsym_colon, which represents a ':' in these contexts:
 *
 * After a label that is the target of a 'goto' statement.
 *
 * In a 'switch' statement, after a 'case' label or a 'default' label.
 *
 * As part of the conditional operator '?:'.
 *
 * In the declaration of a struct member that is a bit-field.
 *
 * Since C11, in the _Generic selection to separate the type from its
 * corresponding expression.
 *
 * See also:
 *	label.c
 *	lsym_case_label.c	for the C11 _Generic expression
 *	lsym_question.c
 */

/*
 * The ':' marks a label that can be used in a 'goto' statement.
 */
//indent input
void endless(void)
{
label1:
goto label2;

    if (true)if (true)if (true)if (true)label2 :goto label1;
}
//indent end

//indent run
void
endless(void)
{
label1:
	goto label2;

	if (true)
		if (true)
			if (true)
				if (true)
			label2:		goto label1;
}
//indent end


/*
 * The ':' is used in a 'switch' statement, after a 'case' label or a
 * 'default' label.
 */
//indent input
void
example(void)
{
	switch (expr) {
	case 'x':
		return;
	default:
		return;
	}
}
//indent end

//indent run-equals-input


/*
 * The ':' is used as part of the conditional operator '?:'.
 */
//indent input
int constant_expression = true?4:12345;
//indent end

//indent run
int		constant_expression = true ? 4 : 12345;
//indent end


/*
 * The ':' is used in the declaration of a struct member that is a bit-field.
 */
//indent input
struct bit_field {
	bool flag:1;
	int maybe_signed : 4;
	signed int definitely_signed:3;
	signed int : 0;/* padding */
	unsigned int definitely_unsigned:3;
	unsigned int:0;/* padding */
};
//indent end

//indent run
struct bit_field {
	bool		flag:1;
	int		maybe_signed:4;
	signed int	definitely_signed:3;
/* $ XXX: Placing the colon directly at the type looks inconsistent. */
	signed int:	0;	/* padding */
	unsigned int	definitely_unsigned:3;
/* $ XXX: Placing the colon directly at the type looks inconsistent. */
	unsigned int:	0;	/* padding */
};
//indent end
