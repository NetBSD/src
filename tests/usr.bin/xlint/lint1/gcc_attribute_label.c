/*	$NetBSD: gcc_attribute_label.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "gcc_attribute_label.c"

/*
 * Tests for the GCC __attribute__ for labels.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html
 */

/* lint1-extra-flags: -X 351 */

void dead(void);

void
test(int i)
{
	if (i < 1000)
		goto hot;
error:
	__attribute__((__cold__));
	dead();

hot:
	__attribute__((__hot__));
	if (i < 0)
		goto error;
}

/* GCC allows a label to be marked as (possibly) unused. */
void
unused_labels(int x)
{
	switch (x) {
	case 3:
		__attribute__((__unused__))
		break;
	case 4:
		goto label;
	label:
		__attribute__((__unused__))
		return;
	}

	/*
	 * The GCC attributes may only occur after a label; they cannot occur
	 * before an arbitrary statement.
	 */
	__attribute__((__unused__))
	/* expect+1: error: syntax error 'return' [249] */
	return;
}
