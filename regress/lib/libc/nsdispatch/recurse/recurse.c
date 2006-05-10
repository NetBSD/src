/*	$NetBSD: recurse.c,v 1.3 2006/05/10 19:07:22 mrg Exp $	*/

/*
 * Written by Jason R. Thorpe, August 1, 2004.
 * Public domain.
 */

#define	_REENTRANT

#include <assert.h>
#include <nsswitch.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

static const ns_src testsrc[] = {
	{ "test",	NS_SUCCESS },
	{ 0 }
};

static int
func3(void *rv, void *cb_data, va_list ap)
{

	printf("func3: enter\n");
	printf("func3: exit\n");
	return (NS_SUCCESS);
}

static int
func2(void *rv, void *cb_data, va_list ap)
{
	static const ns_dtab dtab[] = {
		{ "test",	func3,		NULL },
		{ 0 }
	};
	int r;

	printf("func2: enter\n");
	r = nsdispatch(NULL, dtab, "test", "test", testsrc);
	printf("func2: exit\n");
	return (r);
}

static int
func1(void)
{
	static const ns_dtab dtab[] = {
		{ "test",	func2,		NULL },
		{ 0 }
	};
	int r;

	printf("func1: enter\n");
	r = nsdispatch(NULL, dtab, "test", "test", testsrc);
	printf("func1: exit\n");
	return (r);
}

static void *
thrfunc(void *arg)
{

	pthread_exit(NULL);
}

int
main(int argc, char *argv[])
{
	pthread_t thr;
	void *threval;

	assert(pthread_create(&thr, NULL, thrfunc, NULL) == 0);
	assert(func1() == NS_SUCCESS);
	assert(pthread_join(thr, &threval) == 0);
	exit(0);
}
