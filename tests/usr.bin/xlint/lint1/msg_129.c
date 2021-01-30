/*	$NetBSD: msg_129.c,v 1.2 2021/01/30 22:38:54 rillig Exp $	*/
# 3 "msg_129.c"

// Test for message: expression has null effect [129]

/* lint1-extra-flags: -h */

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

/*
 * XXX: The message 129 looks wrong in this case.  There are several comma
 * operators, each of them has an assignment as operand, and an assignment
 * has side effects.
 *
 * Nevertheless, when stepping through check_null_effect, the operator ','
 * in line 17 says it has no side effect, which is strange.
 */
void
uint8_buffer_write_uint32(uint8_t *c, uint32_t l)
{
	(*(c++) = (uint8_t)(l & 0xff),
	    *(c++) = (uint8_t)((l >> 8L) & 0xff),
	    *(c++) = (uint8_t)((l >> 16L) & 0xff),
	    *(c++) = (uint8_t)((l >> 24L) & 0xff));	/* expect: 129 */
}
