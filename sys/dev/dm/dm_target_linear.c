/*        $NetBSD: dm_target_linear.c,v 1.37 2020/01/21 16:27:53 tkusumi Exp $      */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dm_target_linear.c,v 1.37 2020/01/21 16:27:53 tkusumi Exp $");

/*
 * This file implements initial version of device-mapper linear target.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/kmem.h>
#include <sys/lwp.h>

#include <machine/int_fmtio.h>

#include "dm.h"

/*
 * Allocate target specific config data, and link them to table.
 * This function is called only when, flags is not READONLY and
 * therefore we can add things to pdev list. This should not a
 * problem because this routine is called only from dm_table_load_ioctl.
 * @argv[0] is name,
 * @argv[1] is physical data offset.
 */
int
dm_target_linear_init(dm_table_entry_t *table_en, int argc, char **argv)
{
	dm_target_linear_config_t *tlc;
	dm_pdev_t *dmp;

	if (argc != 2) {
		printf("Linear target takes 2 args, %d given\n", argc);
		return EINVAL;
	}

	aprint_debug("Linear target init function called %s--%s!!\n",
	    argv[0], argv[1]);

	/* Insert dmp to global pdev list */
	if ((dmp = dm_pdev_insert(argv[0])) == NULL)
		return ENOENT;

	tlc = kmem_alloc(sizeof(dm_target_linear_config_t), KM_SLEEP);
	tlc->pdev = dmp;
	tlc->offset = atoi64(argv[1]);

	dm_table_add_deps(table_en, dmp);
	table_en->target_config = tlc;

	return 0;
}

/*
 * Table routine is called to get params string, which is target
 * specific. When dm_table_status_ioctl is called with flag
 * DM_STATUS_TABLE_FLAG I have to sent params string back.
 */
char *
dm_target_linear_table(void *target_config)
{
	dm_target_linear_config_t *tlc;
	char *params;
	tlc = target_config;

	aprint_debug("Linear target table function called\n");

	params = kmem_alloc(DM_MAX_PARAMS_SIZE, KM_SLEEP);
	snprintf(params, DM_MAX_PARAMS_SIZE, "%s %" PRIu64,
	    tlc->pdev->udev_name, tlc->offset);

	return params;
}

/*
 * Do IO operation, called from dmstrategy routine.
 */
int
dm_target_linear_strategy(dm_table_entry_t *table_en, struct buf *bp)
{
	dm_target_linear_config_t *tlc;

	tlc = table_en->target_config;

	bp->b_blkno += tlc->offset;

	VOP_STRATEGY(tlc->pdev->pdev_vnode, bp);

	return 0;
}

/*
 * Sync underlying disk caches.
 */
int
dm_target_linear_sync(dm_table_entry_t *table_en)
{
	int cmd;
	dm_target_linear_config_t *tlc;

	tlc = table_en->target_config;

	cmd = 1;

	return VOP_IOCTL(tlc->pdev->pdev_vnode,  DIOCCACHESYNC, &cmd,
	    FREAD|FWRITE, kauth_cred_get());
}

/*
 * Destroy target specific data. Decrement table pdevs.
 */
int
dm_target_linear_destroy(dm_table_entry_t *table_en)
{

	/*
	 * Destroy function is called for every target even if it
	 * doesn't have target_config.
	 */
	if (table_en->target_config == NULL)
		goto out;

	dm_target_linear_config_t *tlc = table_en->target_config;

	/* Decrement pdev ref counter if 0 remove it */
	dm_pdev_decr(tlc->pdev);

	kmem_free(tlc, sizeof(*tlc));

out:
	/* Unbusy target so we can unload it */
	dm_target_unbusy(table_en->target);
	return 0;
}

#if 0
/*
 * Register upcall device.
 * Linear target doesn't need any upcall devices but other targets like
 * mirror, snapshot, multipath, stripe will use this functionality.
 */
int
dm_target_linear_upcall(dm_table_entry_t *table_en, struct buf *bp)
{

	return 0;
}
#endif

/*
 * Query physical block size of this target
 * For a linear target this is just the sector size of the underlying device
 */
int
dm_target_linear_secsize(dm_table_entry_t *table_en, unsigned int *secsizep)
{
	dm_target_linear_config_t *tlc;
	unsigned int secsize;

	secsize = 0;

	tlc = table_en->target_config;
	if (tlc != NULL)
		secsize = tlc->pdev->pdev_secsize;

	*secsizep = secsize;

	return 0;
}
