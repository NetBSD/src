/*$NetBSD: dm_target_stripe.c,v 1.6 2009/06/05 19:56:40 haad Exp $*/

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
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

/*
 * This file implements initial version of device-mapper stripe target.
 */
#include <sys/types.h>
#include <sys/param.h>

#include <sys/buf.h>
#include <sys/kmem.h>
#include <sys/vnode.h>

#include "dm.h"
#include "netbsd-dm.h"

#ifdef DM_TARGET_MODULE
/*
 * Every target can be compiled directly to dm driver or as a
 * separate module this part of target is used for loading targets
 * to dm driver.
 * Target can be unloaded from kernel only if there are no users of
 * it e.g. there are no devices which uses that target.
 */
#include <sys/kernel.h>
#include <sys/module.h>

MODULE(MODULE_CLASS_MISC, dm_target_stripe, NULL);

static int
dm_target_stripe_modcmd(modcmd_t cmd, void *arg)
{
	dm_target_t *dmt;
	int r;
	dmt = NULL;
	
	switch (cmd) {
	case MODULE_CMD_INIT:
		if ((dmt = dm_target_lookup("stripe")) != NULL) {
			dm_target_unbusy(dmt);
			return EEXIST;
		}
		
		dmt = dm_target_alloc("stripe");
		
		dmt->version[0] = 1;
		dmt->version[1] = 0;
		dmt->version[2] = 0;
		strlcpy(dmt->name, "stripe", DM_MAX_TYPE_NAME);
		dmt->init = &dm_target_stripe_init;
		dmt->status = &dm_target_stripe_status;
		dmt->strategy = &dm_target_stripe_strategy;
		dmt->deps = &dm_target_stripe_deps;
		dmt->destroy = &dm_target_stripe_destroy;
		dmt->upcall = &dm_target_stripe_upcall;

		r = dm_target_insert(dmt);
		
		break;

	case MODULE_CMD_FINI:
		r = dm_target_rem("stripe");
		break;

	case MODULE_CMD_STAT:
		return ENOTTY;

	default:
		return ENOTTY;
	}

	return r;
}

#endif

/*
 * Init function called from dm_table_load_ioctl.
 * Example line sent to dm from lvm tools when using striped target.
 * start length striped #stripes chunk_size device1 offset1 ... deviceN offsetN
 * 0 65536 striped 2 512 /dev/hda 0 /dev/hdb 0
 */
int
dm_target_stripe_init(dm_dev_t *dmv, void **target_config, prop_dictionary_t dict)
{
	dm_target_stripe_config_t *tsc;
	prop_array_t dev_array;
	prop_dictionary_t dev_dict1, dev_dict2;
	
	uint64_t stripes, chunk_size, offset1, offset2;
	const char *device1, *device2;
	
	if (prop_dictionary_get_uint64(dict, DM_TARGET_STRIPE_STRIPES,
		&stripes) == false)
		return EINVAL;
	
	if (prop_dictionary_get_uint64(dict, DM_TARGET_STRIPE_CHUNKSIZE,
		&chunk_size) == false)
		return EINVAL;
		
	dev_array = prop_dictionary_get(dict, DM_TARGET_STRIPE_DEVARRAY);
	
	/* XXX Support for more than 2 devices */
	dev_dict1 = prop_array_get(dev_array, 0);
	dev_dict2 = prop_array_get(dev_array, 1);

	if(dev_dict2 == NULL || dev_dict1 == NULL)
		return EINVAL;
	
        prop_dictionary_get_cstring_nocopy(dev_dict1, DM_TARGET_STRIPE_DEVICE, &device1);
	prop_dictionary_get_uint64(dev_dict1, DM_TARGET_STRIPE_OFFSET, &offset1);
	prop_dictionary_get_cstring_nocopy(dev_dict2, DM_TARGET_STRIPE_DEVICE, &device2);
	prop_dictionary_get_uint64(dev_dict2, DM_TARGET_STRIPE_OFFSET, &offset2);

	aprint_debug("Stripe target init function called!!\n");

	aprint_debug("Stripe target chunk size %"PRIu64" number of stripes %"PRIu64"\n", chunk_size, stripes);
	aprint_debug("Stripe target device name %s -- offset %"PRIu64"\n", device1, offset1);
	aprint_debug("Stripe target device name %s -- offset %"PRIu64"\n", device2, offset2);

	if (stripes > MAX_STRIPES)
		return ENOTSUP;

	if ((tsc = kmem_alloc(sizeof(dm_target_stripe_config_t), KM_NOSLEEP))
	    == NULL)
		return ENOMEM;
	
	/* Insert dmp to global pdev list */
	if ((tsc->stripe_devs[0].pdev = dm_pdev_insert(device1)) == NULL)
		return ENOENT;

	/* Insert dmp to global pdev list */
	if ((tsc->stripe_devs[1].pdev = dm_pdev_insert(device2)) == NULL)
		return ENOENT;

	tsc->stripe_devs[0].offset = offset1;
	tsc->stripe_devs[1].offset = offset2;

	/* Save length of param string */
	tsc->params_len = DM_MAX_PARAMS_SIZE;
	tsc->stripe_chunksize = chunk_size;
	tsc->stripe_num = (uint8_t)stripes;
	
	*target_config = tsc;

	dmv->dev_type = DM_STRIPE_DEV;
	
	return 0;
}

/* Status routine called to get params string. */
char *
dm_target_stripe_status(void *target_config)
{
	dm_target_stripe_config_t *tsc;
	char *params;

	tsc = target_config;
	
	if ((params = kmem_alloc(tsc->params_len, KM_NOSLEEP)) == NULL)
		return NULL;

	snprintf(params, tsc->params_len, "%d %"PRIu64" %s %"PRIu64" %s %"PRIu64,
	    tsc->stripe_num, tsc->stripe_chunksize,
	    tsc->stripe_devs[0].pdev->name, tsc->stripe_devs[0].offset,
	    tsc->stripe_devs[1].pdev->name, tsc->stripe_devs[1].offset);
	
	return params;
}	

/* Strategy routine called from dm_strategy. */
int
dm_target_stripe_strategy(dm_table_entry_t *table_en, struct buf *bp)
{
	dm_target_stripe_config_t *tsc;
	struct buf *nestbuf;
	uint64_t blkno, blkoff;
	uint64_t stripe, stripe_blknr;
	uint32_t stripe_off, stripe_rest, num_blks, issue_blks;
	int stripe_devnr;

	tsc = table_en->target_config;
	if (tsc == NULL)
		return 0;

/*	aprint_debug("Stripe target read function called %" PRIu64 "!!\n",
	tlc->offset);*/

	/* calculate extent of request */
	KASSERT(bp->b_resid % DEV_BSIZE == 0);

	blkno  = bp->b_blkno;
	blkoff = 0;
	num_blks = bp->b_resid / DEV_BSIZE;
	for (;;) {
		/* blockno to strip piece nr */
		stripe     = blkno / tsc->stripe_chunksize;
		stripe_off = blkno % tsc->stripe_chunksize;

		/* where we are inside the strip */
		stripe_devnr = stripe % tsc->stripe_num;
		stripe_blknr = stripe / tsc->stripe_num;

		/* how much is left before we hit a boundary */
		stripe_rest = tsc->stripe_chunksize - stripe_off;

		/* issue this piece on stripe `stripe' */
		issue_blks = MIN(stripe_rest, num_blks);
		nestbuf = getiobuf(NULL, true);

		nestiobuf_setup(bp, nestbuf, blkoff, issue_blks * DEV_BSIZE);
		nestbuf->b_blkno = stripe_blknr * tsc->stripe_chunksize + stripe_off;
		nestbuf->b_blkno += tsc->stripe_devs[stripe_devnr].offset;

		VOP_STRATEGY(tsc->stripe_devs[stripe_devnr].pdev->pdev_vnode, nestbuf);

		blkno    += issue_blks;
		blkoff   += issue_blks * DEV_BSIZE;
		num_blks -= issue_blks;

		if (num_blks <= 0)
			break;
	}

	return 0;
}

/* Doesn't do anything here. */
int
dm_target_stripe_destroy(dm_table_entry_t *table_en)
{
	dm_target_stripe_config_t *tsc;
		
	tsc = table_en->target_config;

	if (tsc == NULL)
		return 0;
	
	dm_pdev_decr(tsc->stripe_devs[0].pdev);
	dm_pdev_decr(tsc->stripe_devs[1].pdev);

	/* Unbusy target so we can unload it */
	dm_target_unbusy(table_en->target);
	
	kmem_free(tsc, sizeof(dm_target_stripe_config_t));
	
	table_en->target_config = NULL;

	return 0;
}

/* Doesn't not need to do anything here. */
int
dm_target_stripe_deps(dm_table_entry_t *table_en, prop_array_t prop_array)
{	
	dm_target_stripe_config_t *tsc;
	struct vattr va;
	
	int error;
	
	if (table_en->target_config == NULL)
		return ENOENT;
	
	tsc = table_en->target_config;
	
	if ((error = VOP_GETATTR(tsc->stripe_devs[0].pdev->pdev_vnode, &va, curlwp->l_cred)) != 0)
		return error;

	prop_array_add_uint64(prop_array, (uint64_t)va.va_rdev);
	
	if ((error = VOP_GETATTR(tsc->stripe_devs[1].pdev->pdev_vnode, &va, curlwp->l_cred)) != 0)
		return error;
	
	prop_array_add_uint64(prop_array, (uint64_t)va.va_rdev);
	
	return 0;
}

/* Unsupported for this target. */
int
dm_target_stripe_upcall(dm_table_entry_t *table_en, struct buf *bp)
{
	return 0;
}
