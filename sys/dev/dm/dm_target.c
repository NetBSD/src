/*        $NetBSD: dm_target.c,v 1.1.2.11 2008/09/03 22:50:17 haad Exp $      */

/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code Must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/types.h>
#include <sys/param.h>

#include <sys/errno.h>
#include <sys/kmem.h>

#include "netbsd-dm.h"
#include "dm.h"

static struct dm_target* dm_target_alloc(const char *);

TAILQ_HEAD(dm_target_head, dm_target);

static struct dm_target_head dm_target_list =
TAILQ_HEAD_INITIALIZER(dm_target_list);

/*
 * Search for name in TAIL and return apropriate pointer.
 */
struct dm_target*
dm_target_lookup_name(const char *dm_target_name)
{
	struct dm_target *dm_target;
        int dlen; int slen;

	slen = strlen(dm_target_name)+1;

	TAILQ_FOREACH (dm_target, &dm_target_list, dm_target_next) {
		dlen = strlen(dm_target->name)+1;

		if (dlen != slen)
			continue;
		
		if (strncmp(dm_target_name, dm_target->name, slen) == 0)
					return dm_target;
	}

	return NULL;
}

/*
 * Insert new target struct into the TAIL.
 * dm_target
 *   contains name, version, function pointer to specifif target functions.
 */
int
dm_target_insert(struct dm_target *dm_target)
{
	struct dm_target *dmt;

	dmt = dm_target_lookup_name(dm_target->name);

	if (dmt != NULL) {
		if (dmt->version[0] >= dm_target->version[0] &&
		    dmt->version[1] >= dm_target->version[1] &&
		    dmt->version[2] >= dm_target->version[2])
				return EEXIST;
	}
	
	TAILQ_INSERT_TAIL(&dm_target_list, dm_target, dm_target_next);
	
	return 0;
}


/*
 * Remove target from TAIL, target is selected with it's name.
 */
int
dm_target_rem(char *dm_target_name)
{
	struct dm_target *dm_target;
	
	if (dm_target_name == NULL)
		return ENOENT;
		    
	dm_target = dm_target_lookup_name(dm_target_name);
	if (dm_target == NULL)
		return ENOENT;
	
	TAILQ_REMOVE(&dm_target_list,
	    dm_target, dm_target_next);
	
	(void)kmem_free(dm_target, sizeof(struct dm_target));

	return 0;
}

/*
 * Destroy all targets and remove them from queue.
 * This routine is called from dm_detach, before module
 * is unloaded.
 */

int
dm_target_destroy(void)
{
	struct dm_target *dm_target;

	while (TAILQ_FIRST(&dm_target_list) != NULL){

		dm_target = TAILQ_FIRST(&dm_target_list);
		
		TAILQ_REMOVE(&dm_target_list, TAILQ_FIRST(&dm_target_list),
		dm_target_next);
		
		(void)kmem_free(dm_target, sizeof(struct dm_target));
	}
	
	return 0;
}

/*
 * Allocate new target entry.
 */
struct dm_target*
dm_target_alloc(const char *name)
{
	struct dm_target *dmt;
	
	if ((dmt = kmem_alloc(sizeof(struct dm_target), KM_NOSLEEP)) == NULL)
		return NULL;
	
	return dmt;
}

/*
 * Return prop_array of dm_target dictionaries.
 */
prop_array_t
dm_target_prop_list(void)
{
	prop_array_t target_array,ver;
	prop_dictionary_t target_dict;
	struct dm_target *dm_target;

	size_t i,j;

	j = 0;
	
	target_array = prop_array_create();
	
	TAILQ_FOREACH (dm_target, &dm_target_list, dm_target_next){

		target_dict  = prop_dictionary_create();
		ver = prop_array_create();
		
		prop_dictionary_set_cstring(target_dict, DM_TARGETS_NAME,
		    dm_target->name);

		for (i=0;i<3;i++)
			prop_array_set_uint32(ver, i, dm_target->version[i]);

		prop_dictionary_set(target_dict, DM_TARGETS_VERSION, ver);

		prop_array_set(target_array, j, target_dict);
		
		prop_object_release(target_dict);

		j++;
	}

	return target_array;
}

/* Initialize dm_target subsystem. */
int
dm_target_init(void)
{
	struct dm_target *dmt,*dmt1,*dmt2,*dmt3,*dmt4,*dmt5,*dmt6;
	int r;

	r = 0;
	
	dmt = dm_target_alloc("linear");
	dmt1 = dm_target_alloc("zero");
	dmt2 = dm_target_alloc("error");
	dmt3 = dm_target_alloc("striped");
	dmt4 = dm_target_alloc("mirror");
	dmt5 = dm_target_alloc("snapshot");
	dmt6 = dm_target_alloc("snapshot-origin");
	
	dmt->version[0] = 1;
	dmt->version[1] = 0;
	dmt->version[2] = 2;
	strlcpy(dmt->name, "linear", DM_MAX_TYPE_NAME);
	dmt->init = &dm_target_linear_init;
	dmt->status = &dm_target_linear_status;
	dmt->strategy = &dm_target_linear_strategy;
	dmt->destroy = &dm_target_linear_destroy;
	dmt->upcall = &dm_target_linear_upcall;
	
	r = dm_target_insert(dmt);
		
	dmt1->version[0] = 1;
	dmt1->version[1] = 0;
	dmt1->version[2] = 0;
	strlcpy(dmt1->name, "zero", DM_MAX_TYPE_NAME);
	dmt1->init = &dm_target_zero_init;
	dmt1->status = &dm_target_zero_status;
	dmt1->strategy = &dm_target_zero_strategy;
	dmt1->destroy = &dm_target_zero_destroy; 
	dmt1->upcall = &dm_target_zero_upcall;
	
	r = dm_target_insert(dmt1);

	dmt2->version[0] = 1;
	dmt2->version[1] = 0;
	dmt2->version[2] = 0;
	strlcpy(dmt2->name, "error", DM_MAX_TYPE_NAME);
	dmt2->init = &dm_target_error_init;
	dmt2->status = &dm_target_error_status;
	dmt2->strategy = &dm_target_error_strategy;
	dmt2->destroy = &dm_target_error_destroy; 
	dmt2->upcall = &dm_target_error_upcall;
	
	r = dm_target_insert(dmt2);
	
	dmt3->version[0] = 1;
	dmt3->version[1] = 0;
	dmt3->version[2] = 3;
	strlcpy(dmt3->name, "striped", DM_MAX_TYPE_NAME);
	dmt3->init = &dm_target_linear_init;
	dmt3->status = &dm_target_linear_status;
	dmt3->strategy = &dm_target_linear_strategy;
	dmt3->destroy = &dm_target_linear_destroy;
	dmt3->upcall = NULL;
	
	r = dm_target_insert(dmt3);

	dmt4->version[0] = 1;
	dmt4->version[1] = 0;
	dmt4->version[2] = 3;
	strlcpy(dmt4->name, "mirror", DM_MAX_TYPE_NAME);
	dmt4->init = NULL;
	dmt4->status = NULL;
	dmt4->strategy = NULL;
	dmt4->destroy = NULL; 
	dmt4->upcall = NULL;
	
	r = dm_target_insert(dmt4);

	dmt5->version[0] = 1;
	dmt5->version[1] = 0;
	dmt5->version[2] = 5;
	strlcpy(dmt5->name, "snapshot", DM_MAX_TYPE_NAME);
	dmt5->init = &dm_target_snapshot_init;
	dmt5->status = &dm_target_snapshot_status;
	dmt5->strategy = &dm_target_snapshot_strategy;
	dmt5->destroy = &dm_target_snapshot_destroy;
	dmt5->upcall = &dm_target_snapshot_upcall;
	
	r = dm_target_insert(dmt5);
	
	dmt6->version[0] = 1;
	dmt6->version[1] = 0;
	dmt6->version[2] = 5;
	strlcpy(dmt6->name, "snapshot-origin", DM_MAX_TYPE_NAME);
	dmt6->init = &dm_target_snapshot_orig_init;
	dmt6->status = &dm_target_snapshot_orig_status;
	dmt6->strategy = &dm_target_snapshot_orig_strategy;
	dmt6->destroy = &dm_target_snapshot_orig_destroy;
	dmt6->upcall = &dm_target_snapshot_orig_upcall;

	r = dm_target_insert(dmt6);
	
	return r;
}
