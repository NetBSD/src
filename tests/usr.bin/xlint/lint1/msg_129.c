/*	$NetBSD: msg_129.c,v 1.6 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_129.c"

// Test for message: expression has null effect [129]

/* lint1-extra-flags: -h */

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

_Bool side_effect(void);

/*
 * Before tree.c 1.198 from 2021-01-30, the nested comma operators were
 * wrongly reported as having no side effect.
 *
 * The bug was that has_side_effect did not properly examine the sub-nodes.
 * The ',' operator itself has m_has_side_effect == false since it depends
 * on its operands whether the ',' actually has side effects.  For nested ','
 * operators, the function did not evaluate the operands deeply but only did
 * a quick shallow test on the m_has_side_effect property.  Since that is
 * false, lint thought that the whole expression would have no side effect.
 */
void
uint8_buffer_write_uint32(uint8_t *c, uint32_t l)
{
	(*(c++) = (uint8_t)(l & 0xff),
	    *(c++) = (uint8_t)((l >> 8L) & 0xff),
	    *(c++) = (uint8_t)((l >> 16L) & 0xff),
	    *(c++) = (uint8_t)((l >> 24L) & 0xff));
}

void
operator_comma(void)
{
	side_effect(), 0;		/* the 0 is redundant */
	/* expect+1: warning: expression has null effect [129] */
	0, side_effect();

	if (side_effect(), 0)		/* the 0 controls the 'if' */
		return;
	/* expect+1: warning: expression has null effect [129] */
	if (0, side_effect())
		return;
}

void
legitimate_use_cases(int arg)
{
	int local = 3;

	/*
	 * This expression is commonly used to mark the argument as
	 * deliberately unused.
	 */
	(void)arg;

	/*
	 * This expression is commonly used to mark the local variable as
	 * deliberately unused.  This situation occurs when the local
	 * variable is only used in some but not all compile-time selectable
	 * variants of the code, such as in debugging mode, and writing down
	 * the exact conditions would complicate the code unnecessarily.
	 */
	(void)local;

	/* This is a short-hand notation for a do-nothing command. */
	(void)0;

	/*
	 * At the point where lint checks for expressions having a null
	 * effect, constants have been folded, therefore the following
	 * expression is considered safe as well.  It does not appear in
	 * practice though.
	 */
	(void)(3 - 3);

	/*
	 * This variant of the do-nothing command is commonly used in
	 * preprocessor macros since it works nicely with if-else and if-then
	 * statements.  It is longer than the above variant though.
	 */
	do {
	} while (0);

	/*
	 * Only the expression '(void)0' is common, other expressions are
	 * unusual enough to warrant a warning.
	 */
	/* expect+1: warning: expression has null effect [129] */
	(void)13;

	/* Double casts are unusual enough to warrant a warning. */
	/* expect+1: warning: expression has null effect [129] */
	(void)(void)0;
}
