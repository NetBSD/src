/*	$NetBSD: msg_128.c,v 1.10 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_128.c"

// Test for message: operator '%s' discards '%s' from '%s' [128]

/* lint1-extra-flags: -X 351 */

char *ptr;
const char *cptr;
volatile char *vptr;
const volatile char *cvptr;

const volatile int *cviptr;

void
assign(void)
{
	/* expect+1: warning: operator '=' discards 'const volatile' from 'pointer to const volatile char' [128] */
	ptr = cvptr;
	/* expect+1: warning: operator '=' discards 'volatile' from 'pointer to const volatile char' [128] */
	cptr = cvptr;
	/* expect+1: warning: operator '=' discards 'const' from 'pointer to const volatile char' [128] */
	vptr = cvptr;

	/* expect+1: warning: invalid combination of 'pointer to char' and 'pointer to const volatile int', op '=' [124] */
	ptr = cviptr;
	/* expect+1: warning: invalid combination of 'pointer to const char' and 'pointer to const volatile int', op '=' [124] */
	cptr = cviptr;
	/* expect+1: warning: invalid combination of 'pointer to volatile char' and 'pointer to const volatile int', op '=' [124] */
	vptr = cviptr;
}
