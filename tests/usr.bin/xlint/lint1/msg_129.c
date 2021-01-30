/*	$NetBSD: msg_129.c,v 1.3 2021/01/30 23:05:08 rillig Exp $	*/
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
 * The ',' operator has m_has_side_effect == false since it depends on its
 * operands whether the ',' actually has side effects.  For nested ','
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
	0, side_effect();		/* expect: 129 */

	if (side_effect(), 0)		/* the 0 controls the 'if' */
		return;
	if (0, side_effect())		/* expect: 129 */
		return;
}
