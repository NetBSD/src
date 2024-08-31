/*	$NetBSD: msg_361.c,v 1.3 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_361.c"

// Test for message: number base '%.*s' is %ju, must be 8, 10 or 16 [361]

/*
 * The first or second character of the snprintb format specifies the number
 * base.  It must be given in binary.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
old_style_number_base(void)
{
	char buf[64];

	/* expect+1: warning: missing new-style '\177' or old-style number base [359] */
	snprintb(buf, sizeof(buf), "", 0);
	/* expect+1: warning: number base '\002' is 2, must be 8, 10 or 16 [361] */
	snprintb(buf, sizeof(buf), "\002", 0);
	snprintb(buf, sizeof(buf), "\010", 0);
	snprintb(buf, sizeof(buf), "\n", 0);
	snprintb(buf, sizeof(buf), "\020", 0);
	/* expect+1: warning: number base '\014' is 12, must be 8, 10 or 16 [361] */
	snprintb(buf, sizeof(buf), "" "\014" "", 0);
	snprintb(buf, sizeof(buf), "" "\020" "", 0);
}

void
new_style_number_base(void)
{
	char buf[64];

	/* expect+1: warning: missing new-style number base after '\177' [360] */
	snprintb(buf, sizeof(buf), "\177", 0);
	/* expect+1: warning: number base '\0' is 0, must be 8, 10 or 16 [361] */
	snprintb(buf, sizeof(buf), "\177\0", 0);
	/* expect+1: warning: number base '\002' is 2, must be 8, 10 or 16 [361] */
	snprintb(buf, sizeof(buf), "\177\002", 0);
	snprintb(buf, sizeof(buf), "\177\010", 0);
	snprintb(buf, sizeof(buf), "\177\n", 0);
	snprintb(buf, sizeof(buf), "\177\020", 0);
	/* expect+1: warning: number base '\014' is 12, must be 8, 10 or 16 [361] */
	snprintb(buf, sizeof(buf), "" "\177\014" "", 0);
	snprintb(buf, sizeof(buf), "" "\177\020" "", 0);
}
