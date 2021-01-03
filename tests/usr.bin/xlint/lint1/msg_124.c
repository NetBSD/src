/*	$NetBSD: msg_124.c,v 1.2 2021/01/03 15:44:35 rillig Exp $	*/
# 3 "msg_124.c"

// Test for message: illegal pointer combination, op %s [124]

typedef void(*signal_handler)(int);

typedef signal_handler(*sys_signal)(signal_handler);

typedef int(*printflike)(const char *, ...)
    __attribute__((format(printf, 1, 2)));

void
example(int *ptr)
{
	signal_handler handler = ptr;
	sys_signal signal = ptr;
	printflike printf = ptr;
}
