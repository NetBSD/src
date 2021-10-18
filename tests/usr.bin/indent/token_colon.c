/* $NetBSD: token_colon.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for formatting of the colon token, which is used in the following
 * contexts:
 *
 * After a label that is the target of a goto statement.
 *
 * In a switch statement, after a case label or the default label.
 *
 * As part of the conditional expression operator '?:'.
 *
 * In the declaration of a struct member that is a bit-field.
 */

#indent input
void endless(void)
{
label1:
goto label2;

    if (true)if (true)if (true)if (true)label2 :goto label1;
}
#indent end

#indent run
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
#indent end

#indent input
int constant_expression = true?4:12345;
#indent end

#indent run
int		constant_expression = true ? 4 : 12345;
#indent end

#indent input
struct bit_field {
bool flag:1;
int maybe_signed : 4;
signed int definitely_signed:3;
signed int : 0;/* finish the storage unit for the bit-field */
unsigned int definitely_unsigned:3;
unsigned int:0;/* finish the storage unit for the bit-field */
};
#indent end

#indent run
struct bit_field {
	bool		flag:1;
	int		maybe_signed:4;
	signed int	definitely_signed:3;
/* $ XXX: Placing the colon directly at the type looks inconsistent. */
	signed int:	0;	/* finish the storage unit for the bit-field */
	unsigned int	definitely_unsigned:3;
/* $ XXX: Placing the colon directly at the type looks inconsistent. */
	unsigned int:	0;	/* finish the storage unit for the bit-field */
};
#indent end
