/*	$NetBSD: config_hook.c,v 1.2.4.2 2002/02/28 04:09:34 nathanw Exp $	*/

/*-
 * Copyright (c) 1999-2001
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/config_hook.h>

struct hook_rec {
	LIST_ENTRY(hook_rec) hr_link;
	void *hr_ctx;
	int hr_type;
	long hr_id;
	enum config_hook_mode hr_mode;
	int (*hr_func)(void *, int, long, void *);
};

LIST_HEAD(hook_list, hook_rec);
struct hook_list hook_lists[CONFIG_HOOK_NTYPES];
struct hook_list call_list;

void
config_hook_init()
{
	int i;

	for (i = 0; i < CONFIG_HOOK_NTYPES; i++) {
		LIST_INIT(&hook_lists[i]);
	}
	LIST_INIT(&call_list);
}

config_hook_tag
config_hook(int type, long id, enum config_hook_mode mode,
    int (*func)(void *, int, long, void *), void *ctx)
{
	struct hook_rec *hr, *cr, *prev_hr;
	int s;

	/* check type value */
	if (type < 0 || CONFIG_HOOK_NTYPES <= type) {
		panic("config_hook: invalid hook type");
	}

	/* check mode compatibility */
	prev_hr = NULL;
	for (hr = LIST_FIRST(&hook_lists[type]); hr != NULL;
	    hr = LIST_NEXT(hr, hr_link)) {
		if (hr->hr_id == id) {
			if (hr->hr_mode != mode) {
				panic("config_hook: incompatible mode on "
				    "type=%d/id=%ld != %d",
				    type, id, hr->hr_mode);
			}
			prev_hr = hr;
		}
	}
	switch (mode) {
	case CONFIG_HOOK_SHARE:
		/* nothing to do */
		break;
	case CONFIG_HOOK_REPLACE:
		if (prev_hr != NULL) {
			printf("config_hook: type=%d/id=%ld is replaced",
			    type, id);
			s = splhigh();
			LIST_REMOVE(prev_hr, hr_link);
			prev_hr->hr_link.le_next = NULL;
			splx(s);
		}
		break;
	case CONFIG_HOOK_EXCLUSIVE:
		if (prev_hr != NULL) {
			panic("config_hook: type=%d/id=%ld is already "
			    "hooked(%p)", type, id, prev_hr);
		}
		break;
	default:
		break;
	}

	/* allocate new record */
	hr = malloc(sizeof(*hr), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (hr == NULL)
		panic("config_hook: malloc failed");

	hr->hr_ctx = ctx;
	hr->hr_type = type;
	hr->hr_id = id;
	hr->hr_func = func;
	hr->hr_mode = mode;

	s = splhigh();
	LIST_INSERT_HEAD(&hook_lists[type], hr, hr_link);

	/* update call list */
	for (cr = LIST_FIRST(&call_list); cr != NULL;
	    cr = LIST_NEXT(cr, hr_link)) {
		if (cr->hr_type == type && cr->hr_id == id) {
			if (cr->hr_func != NULL &&
			    cr->hr_mode != mode) {
				panic("config_hook: incompatible mode on "
				    "type=%d/id=%ld != %d",
				    type, id, cr->hr_mode);
			}
			cr->hr_ctx = ctx;
			cr->hr_func = func;
			cr->hr_mode = mode;
		}
	}
	splx(s);

	return (hr);
}

void
config_unhook(config_hook_tag hrx)
{
	int s;
	struct hook_rec *hr = (struct hook_rec*)hrx, *cr;

	if (hr->hr_link.le_next != NULL) {
		s = splhigh();
		LIST_REMOVE(hr, hr_link);
		hr->hr_link.le_next = NULL;
		/* update call list */
		for (cr = LIST_FIRST(&call_list); cr != NULL;
		    cr = LIST_NEXT(cr, hr_link)) {
			if (cr->hr_type == hr->hr_type &&
			    cr->hr_id == hr->hr_id)
				cr->hr_func = NULL;
		}
		splx(s);
	}
	free(hr, M_DEVBUF);
}

int
config_hook_call(int type, long id, void *msg)
{
	int res;
	struct hook_rec *hr;

	/* Check type value. */
	if (type < 0 || CONFIG_HOOK_NTYPES <= type) {
		panic("config_hook: invalid hook type");
	}

	res = -1;
	for (hr = LIST_FIRST(&hook_lists[type]); hr != NULL;
	    hr = LIST_NEXT(hr, hr_link)) {
		if (hr->hr_id == id) {
			res = (*hr->hr_func)(hr->hr_ctx, type, id, msg);
		}
	}

	return (res);
}

config_hook_tag
config_connect(int type, long id)
{
	int s;
	struct hook_rec *cr, *hr;

	/* check type value */
	if (type < 0 || CONFIG_HOOK_NTYPES <= type) {
		panic("config_hook: invalid hook type");
	}

	/* allocate new record */
	cr = malloc(sizeof(*hr), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (cr == NULL)
		panic("config_connect: malloc failed");

	cr->hr_func = NULL;
	cr->hr_type = type;
	cr->hr_id = id;

	s = splhigh();
	/* insert the record into the call list */
	LIST_INSERT_HEAD(&call_list, cr, hr_link);

	/* scan hook list */
	for (hr = LIST_FIRST(&hook_lists[type]); hr != NULL;
	    hr = LIST_NEXT(hr, hr_link)) {
		if (hr->hr_id == id) {
			if (hr->hr_mode == CONFIG_HOOK_SHARE)
				panic("config_connect: can't connect with "
				    "shared hook, type=%d id=%ld\n", type, id);
			cr->hr_ctx = hr->hr_ctx;
			cr->hr_func = hr->hr_func;
			cr->hr_mode = hr->hr_mode;
		}
	}
	splx(s);

	return (cr);
}

void
config_disconnect(config_call_tag crx)
{
	int s;
	struct hook_rec *cr = (struct hook_rec*)crx;

	s = splhigh();
	LIST_REMOVE(cr, hr_link);
	splx(s);

	free(cr, M_DEVBUF);
}

int
config_connected_call(config_call_tag crx, void *msg)
{
	int res;
	struct hook_rec *cr = (struct hook_rec*)crx;

	if (cr->hr_func != NULL)
		res = (*cr->hr_func)(cr->hr_ctx, cr->hr_type, cr->hr_id, msg);
	else
		res = -1;

	return (res);
}
