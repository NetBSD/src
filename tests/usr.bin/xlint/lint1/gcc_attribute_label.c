/*	$NetBSD: gcc_attribute_label.c,v 1.3 2022/04/28 21:38:38 rillig Exp $	*/
# 3 "gcc_attribute_label.c"

/*
 * Tests for the GCC __attribute__ for labels.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html
 */

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
