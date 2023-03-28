/*	$NetBSD: msg_158.c,v 1.6 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_158.c"

// Test for message: '%s' may be used before set [158]

/* lint1-extra-flags: -X 351 */

void sink_int(int);

void
example(int arg)
{
	int twice_arg;

	/* expect+1: warning: 'twice_arg' may be used before set [158] */
	sink_int(twice_arg);
	twice_arg = 2 * arg;
	sink_int(twice_arg);
}

void
conditionally_used(int arg)
{
	int twice_arg;

	if (arg > 0)
		twice_arg = 2 * arg;
	if (arg > 0)
		sink_int(twice_arg);
}

void
conditionally_unused(int arg)
{
	int twice_arg;

	if (arg > 0)
		twice_arg = 2 * arg;

	/*
	 * This situation is not detected by lint as it does not track the
	 * possible code paths for all conditions.
	 */
	if (arg < 0)
		sink_int(twice_arg);
}
