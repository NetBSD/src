/*	$NetBSD: msg_124.c,v 1.3 2021/01/24 10:50:42 rillig Exp $	*/
# 3 "msg_124.c"

// Test for message: illegal pointer combination, op %s [124]

typedef void(*signal_handler)(int);

typedef signal_handler(*sys_signal)(signal_handler);

typedef int(*printflike)(const char *, ...)
    __attribute__((format(printf, 1, 2)));

void
example(int *ptr)
{
	signal_handler handler = ptr;	/* expect: 124 */
	sys_signal signal = ptr;	/* expect: 124 */
	printflike printf = ptr;	/* expect: 124 */
}

void ok(_Bool);
void not_ok(_Bool);

void
compare_pointers(const void *vp, const char *cp, const int *ip,
		 signal_handler fp)
{
	ok(vp == cp);
	ok(vp == ip);
	ok(vp == fp);
	not_ok(cp == ip);	/* expect: 124 */
	not_ok(cp == fp);	/* expect: 124 */
	ok(vp == (void *)0);
	ok(cp == (void *)0);
	ok(ip == (void *)0);
	ok(fp == (void *)0);
	ok(vp == 0);
	ok(cp == 0);
	ok(ip == 0);
	ok(fp == 0);
	ok(vp == 0L);
	ok(cp == 0L);
	ok(ip == 0L);
	ok(fp == 0L);
}
