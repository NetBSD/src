
static short tw_on;
static short tw_pos;
static char tw_chars[4] = "|/-\\";

twiddle()
{
	if (tw_on)
		putchar('\b');
	else
		tw_on = 1;
	putchar(tw_chars[tw_pos++]);
	tw_pos &= 3;
}

reset_twiddle()
{
	if (tw_on)
		putchar('\b');
	tw_on = 0;
	tw_pos = 0;
}

