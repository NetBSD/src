/*	$NetBSD: msg_306.c,v 1.6 2025/02/20 21:53:28 rillig Exp $	*/
# 3 "msg_306.c"

// Test for message: constant %s truncated by conversion, op '%s' [306]

/* lint1-extra-flags: -X 351 */

signed char s8;
unsigned char u8;

void
msg_306(void)
{
	u8 = 0xff;
	/* expect+1: warning: constant truncated by assignment [165] */
	u8 = 0x100;

	u8 &= 0xff;
	/* expect+1: warning: constant 0x100 truncated by conversion, op '&=' [306] */
	u8 &= 0x100;
	/* XXX: Lint doesn't care about the expanded form of the same code. */
	u8 = u8 & 0x100;

	u8 |= 0xff;
	/* expect+1: warning: constant 0x100 truncated by conversion, op '|=' [306] */
	u8 |= 0x100;
	/* XXX: Lint doesn't care about the expanded form of the same code. */
	u8 = u8 | 0x100;

	s8 &= 0xff;
	/* expect+1: warning: constant 0x100 truncated by conversion, op '&=' [306] */
	s8 &= 0x100;
	/* XXX: Lint doesn't care about the expanded form of the same code. */
	s8 = s8 & 0x100;
	s8 |= 0xff;
	/* expect+1: warning: constant 0x100 truncated by conversion, op '|=' [306] */
	s8 |= 0x100;
	/* XXX: Lint doesn't care about the expanded form of the same code. */
	s8 = s8 | 0x100;
}
