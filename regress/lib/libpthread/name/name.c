/*	$NetBSD: name.c,v 1.2 2003/02/27 15:18:41 hannken Exp $	*/

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *threadfunc(void *arg);

#define	NAME_TOO_LONG	"12345678901234567890123456789012"	/* 32 chars */
#define	NAME_JUST_RIGHT	"1234567890123456789012345678901"	/* 31 chars */

#define	CONST_NAME	"xyzzy"
char non_const_name[] = CONST_NAME;

int
main(int argc, char *argv[])
{
	pthread_t thr, self = pthread_self();
	pthread_attr_t attr;
	char retname[32];
	int ret;

	pthread_attr_init(&attr);
	assert(0 == pthread_attr_getname_np(&attr, retname, sizeof(retname),
	    NULL));
	assert(retname[0] == '\0');
	assert(EINVAL == pthread_attr_setname_np(&attr, NAME_TOO_LONG, NULL));
	assert(0 == pthread_attr_setname_np(&attr, "%s", NAME_JUST_RIGHT));

	strcpy(retname, "foo");
	assert(0 == pthread_getname_np(self, retname, sizeof(retname)));
	assert(retname[0] == '\0');

	ret = pthread_create(&thr, &attr, threadfunc, NULL);
	if (ret != 0)
		err(1, "pthread_create");

	ret = pthread_join(thr, NULL);
	if (ret != 0)
		err(1, "pthread_join");

	assert(ESRCH == pthread_getname_np(thr, retname, sizeof(retname)));

	return 0;
}

void *
threadfunc(void *arg)
{
	pthread_t self = pthread_self();
	char retname[32];

	assert(0 == pthread_getname_np(self, retname, sizeof(retname)));
	assert(0 == strcmp(retname, NAME_JUST_RIGHT));

	assert(0 == pthread_setname_np(self, non_const_name, NULL));
	memset(non_const_name, 0, sizeof(non_const_name));
	assert(0 == pthread_getname_np(self, retname, sizeof(retname)));
	assert(0 == strcmp(retname, CONST_NAME));

	return NULL;
}
