/*	$NetBSD: pthread_attr.c,v 1.2 2003/09/11 21:51:57 christos Exp $	*/

/*-
 * Copyright (c) 2001,2002,2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_attr.c,v 1.2 2003/09/11 21:51:57 christos Exp $");

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pthread.h"
#include "pthread_int.h"


static struct pthread_attr_private *
pthread__attr_init_private(pthread_attr_t *attr)
{
	struct pthread_attr_private *p;

	if ((p = attr->pta_private) != NULL)
		return p;

	p = malloc(sizeof(*p));
	if (p != NULL) {
		memset(p, 0, sizeof(*p));
		attr->pta_private = p;
	}
	return p;
}


int
pthread_attr_init(pthread_attr_t *attr)
{

	attr->pta_magic = PT_ATTR_MAGIC;
	attr->pta_flags = 0;
	attr->pta_private = NULL;

	return 0;
}


int
pthread_attr_destroy(pthread_attr_t *attr)
{
	struct pthread_attr_private *p;

	if ((p = attr->pta_private) != NULL)
		free(p);

	return 0;
}


int
pthread_attr_get_np(pthread_t thread, pthread_attr_t *attr)
{
	struct pthread_attr_private *p;

	p = pthread__attr_init_private(attr);
	if (p == NULL)
		return ENOMEM;

	attr->pta_flags = thread->pt_flags & 
	    (PT_FLAG_DETACHED | PT_FLAG_SCOPE_SYSTEM | PT_FLAG_EXPLICIT_SCHED);

	p->ptap_namearg = thread->pt_name;
	p->ptap_stackaddr = thread->pt_stack.ss_sp;
	p->ptap_stacksize = thread->pt_stack.ss_size;
	p->ptap_guardsize = (size_t)sysconf(_SC_PAGESIZE);

	return 0;
}


int
pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{

	if (attr->pta_flags & PT_FLAG_DETACHED)
		*detachstate = PTHREAD_CREATE_DETACHED;
	else
		*detachstate = PTHREAD_CREATE_JOINABLE;

	return 0;
}


int
pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{

	switch (detachstate) {
	case PTHREAD_CREATE_JOINABLE:
		attr->pta_flags &= ~PT_FLAG_DETACHED;
		break;
	case PTHREAD_CREATE_DETACHED:
		attr->pta_flags |= PT_FLAG_DETACHED;
		break;
	default:
		return EINVAL;
	}

	return 0;
}


int
pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guard)
{
	struct pthread_attr_private *p;

	if ((p = attr->pta_private) == NULL)
		*guard = (size_t)sysconf(_SC_PAGESIZE);
	else
		*guard = p->ptap_guardsize;

	return 0;
}


int
pthread_attr_setguardsize(pthread_attr_t *attr, size_t guard)
{
	struct pthread_attr_private *p;

	p = pthread__attr_init_private(attr);
	if (p == NULL)
		return ENOMEM;

	p->ptap_guardsize = guard;

	return 0;
}


int
pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inherit)
{

	if (attr->pta_flags & PT_FLAG_EXPLICIT_SCHED)
		*inherit = PTHREAD_EXPLICIT_SCHED;
	else
		*inherit = PTHREAD_INHERIT_SCHED;

	return 0;
}


int
pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit)
{

	switch (inherit) {
	case PTHREAD_INHERIT_SCHED:
		attr->pta_flags &= ~PT_FLAG_EXPLICIT_SCHED;
		break;
	case PTHREAD_EXPLICIT_SCHED:
		attr->pta_flags |= PT_FLAG_EXPLICIT_SCHED;
		break;
	default:
		return EINVAL;
	}

	return 0;
}


int
pthread_attr_getscope(const pthread_attr_t *attr, int *scope)
{

	if (attr->pta_flags & PT_FLAG_SCOPE_SYSTEM)
		*scope = PTHREAD_SCOPE_SYSTEM;
	else
		*scope = PTHREAD_SCOPE_PROCESS;

	return 0;
}


int
pthread_attr_setscope(pthread_attr_t *attr, int scope)
{

	switch (scope) {
	case PTHREAD_SCOPE_PROCESS:
		attr->pta_flags &= ~PT_FLAG_SCOPE_SYSTEM;
		break;
	case PTHREAD_SCOPE_SYSTEM:
		attr->pta_flags |= PT_FLAG_SCOPE_SYSTEM;
		break;
	default:
		return EINVAL;
	}

	return 0;
}


int
/*ARGSUSED*/
pthread_attr_setschedparam(pthread_attr_t *attr,
    const struct sched_param *param)
{

	if (param == NULL)
		return EINVAL;

	if (param->sched_priority != 0)
		return EINVAL;

	return 0;
}


int
/*ARGSUSED*/
pthread_attr_getschedparam(const pthread_attr_t *attr,
    struct sched_param *param)
{

	if (param == NULL)
		return EINVAL;

	param->sched_priority = 0;

	return 0;
}


int
pthread_attr_getstack(const pthread_attr_t *attr, void **addr, size_t *size)
{
	struct pthread_attr_private *p;

	if ((p = attr->pta_private) == NULL) {
		*addr = NULL;
		*size = PT_STACKSIZE;
	} else {
		*addr = p->ptap_stackaddr;
		*size = p->ptap_stacksize;
	}

	return 0;
}


int
pthread_attr_setstack(pthread_attr_t *attr, void *addr, size_t size)
{
	struct pthread_attr_private *p;

	p = pthread__attr_init_private(attr);
	if (p == NULL)
		return ENOMEM;

	p->ptap_stackaddr = addr;
	p->ptap_stacksize = size;

	return 0;
}


int
pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *size)
{
	struct pthread_attr_private *p;

	if ((p = attr->pta_private) == NULL)
		*size = PT_STACKSIZE;
	else
		*size = p->ptap_stacksize;

	return 0;
}


int
pthread_attr_setstacksize(pthread_attr_t *attr, size_t size)
{
	struct pthread_attr_private *p;

	p = pthread__attr_init_private(attr);
	if (p == NULL)
		return ENOMEM;

	p->ptap_stacksize = size;

	return 0;
}


int
pthread_attr_getstackaddr(const pthread_attr_t *attr, void **addr)
{
	struct pthread_attr_private *p;

	if ((p = attr->pta_private) == NULL)
		*addr = NULL;
	else
		*addr = p->ptap_stackaddr;

	return 0;
}


int
pthread_attr_setstackaddr(pthread_attr_t *attr, void *addr)
{
	struct pthread_attr_private *p;

	p = pthread__attr_init_private(attr);
	if (p == NULL)
		return ENOMEM;

	p->ptap_stackaddr = addr;

	return 0;
}


int
pthread_attr_getname_np(const pthread_attr_t *attr, char *name, size_t len,
    void **argp)
{
	struct pthread_attr_private *p;

	if ((p = attr->pta_private) == NULL) {
		name[0] = '\0';
		if (argp != NULL)
			*argp = NULL;
	} else {
		strlcpy(name, p->ptap_name, len);
		if (argp != NULL)
			*argp = p->ptap_namearg;
	}

	return 0;
}


int
pthread_attr_setname_np(pthread_attr_t *attr, const char *name, void *arg)
{
	struct pthread_attr_private *p;
	int namelen;

	p = pthread__attr_init_private(attr);
	if (p == NULL)
		return ENOMEM;

	namelen = snprintf(p->ptap_name, PTHREAD_MAX_NAMELEN_NP, name, arg);
	if (namelen >= PTHREAD_MAX_NAMELEN_NP) {
		p->ptap_name[0] = '\0';
		return EINVAL;
	}
	p->ptap_namearg = arg;

	return 0;
}
