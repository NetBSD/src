/*	$NetBSD: atexit.c,v 1.1 2003/03/01 04:20:54 thorpej Exp $	*/

/*
 * Program to test atexit(3) and __cxa_atexit()/__cxa_finalize().
 *
 * Written by Jason R. Thorpe, February 2003.
 * Public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

extern int __cxa_atexit(void (*func)(void *), void *, void *);
extern void __cxa_finalize(void *);

static int dso_handle_1;
static int dso_handle_2;
static int dso_handle_3;

static int arg_1;
static int arg_2;
static int arg_3;

static int exiting_state;

#define	ASSERT(expr)							\
do {									\
	if ((expr) == 0) {						\
		write(STDERR_FILENO, __func__, strlen(__func__));	\
		write(STDERR_FILENO, ": ", 2);				\
		write(STDERR_FILENO, __STRING(expr),			\
		      strlen(__STRING(expr)));				\
		write(STDERR_FILENO, "\n", 1);				\
		my_abort();						\
	}								\
} while (/*CONSTCOND*/0)

#define	SUCCESS()							\
do {									\
	write(STDOUT_FILENO, __func__, strlen(__func__));		\
	write(STDOUT_FILENO, "\n", 1);					\
} while (/*CONSTCOND*/0)

static void
my_abort(void)
{

	kill(getpid(), SIGABRT);
	/* NOTREACHED */
}

static void
cxa_handler_5(void *arg)
{
	static int cxa_handler_5_called;

	ASSERT(arg == (void *)&arg_1);
	ASSERT(cxa_handler_5_called == 0);
	ASSERT(exiting_state == 5);

	exiting_state--;
	cxa_handler_5_called = 1;
	SUCCESS();
}

static void
cxa_handler_4(void *arg)
{
	static int cxa_handler_4_called;

	ASSERT(arg == (void *)&arg_1);
	ASSERT(cxa_handler_4_called == 0);
	ASSERT(exiting_state == 4);

	exiting_state--;
	cxa_handler_4_called = 1;
	SUCCESS();
}

static void
cxa_handler_3(void *arg)
{
	static int cxa_handler_3_called;

	ASSERT(arg == (void *)&arg_2);
	ASSERT(cxa_handler_3_called == 0);
	ASSERT(exiting_state == 3);

	exiting_state--;
	cxa_handler_3_called = 1;
	SUCCESS();
}

static void
cxa_handler_2(void *arg)
{
	static int cxa_handler_2_called;

	ASSERT(arg == (void *)&arg_3);
	ASSERT(cxa_handler_2_called == 0);
	ASSERT(exiting_state == 2);

	exiting_state--;
	cxa_handler_2_called = 1;
	SUCCESS();
}

static void
normal_handler_1(void)
{
	static int normal_handler_1_called;

	ASSERT(normal_handler_1_called == 0);
	ASSERT(exiting_state == 1);

	exiting_state--;
	normal_handler_1_called = 1;
	SUCCESS();
}

static void
normal_handler_0(void)
{
	static int normal_handler_0_called;

	ASSERT(normal_handler_0_called == 0);
	ASSERT(exiting_state == 0);

	normal_handler_0_called = 1;
	SUCCESS();
}

int
main(int argc, char *argv[])
{

	exiting_state = 5;

	ASSERT(0 == atexit(normal_handler_0));
	ASSERT(0 == atexit(normal_handler_1));
	ASSERT(0 == __cxa_atexit(cxa_handler_4, &arg_1, &dso_handle_1));
	ASSERT(0 == __cxa_atexit(cxa_handler_5, &arg_1, &dso_handle_1));
	ASSERT(0 == __cxa_atexit(cxa_handler_3, &arg_2, &dso_handle_2));
	ASSERT(0 == __cxa_atexit(cxa_handler_2, &arg_3, &dso_handle_3));

	__cxa_finalize(&dso_handle_1);
	__cxa_finalize(&dso_handle_2);
	exit(0);
}
