/*        $NetBSD: dm_target_linear.c,v 1.1.2.12 2008/08/28 22:03:39 haad Exp $      */

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
 * This file implements initial version of device-mapper dklinear target.
 */

#include <sys/types.h>
#include <sys/param.h>

#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/kmem.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/vnode.h>

#include <machine/int_fmtio.h>

#include "dm.h"

/*
 * Allocate target specific config data, and link them to table.
 * This function is called only when, flags is not READONLY and
 * therefore we can add things to pdev list. This function have to
 * be called with held dev_mtx. This should not e a problem because
 * this routine is called onyl from dm_table_load_ioctl.
 * @argv[0] is name,
 * @argv[1] is physical data offset.
 */
int
dm_target_linear_init(struct dm_dev *dmv, void **target_config, char *params)
{
	struct target_linear_config *tlc;
	struct dm_pdev *dmp;

	char **ap, *argv[3];
	
	/*
	 * Parse a string, containing tokens delimited by white space,
	 * into an argument vector
	 */
	for (ap = argv; ap < &argv[9] &&
		 (*ap = strsep(&params, " \t")) != NULL;) {
		if (**ap != '\0')
			ap++;
	}

	/* Insert dmp to global pdev list */
	if ((dmp = dm_pdev_insert(argv[0])) == NULL)
		return ENOENT;
	
	/* Lookup for pdev entry in device pdev list and insert */
	if ((dm_pdev_lookup_name_list(dmp->name, &dmv->pdevs)) == NULL)
		SLIST_INSERT_HEAD(&dmv->pdevs, dmp, next_pdev); 
		
	dmp->list_ref_cnt++;	
	
	printf("Linear target init function called %s--%s!!\n",
	    argv[0], argv[1]);
	
	if ((tlc = kmem_alloc(sizeof(struct target_linear_config), KM_NOSLEEP))
	    == NULL)
		return 1;

	tlc->pdev = dmp;
	tlc->offset = 0; 	/* default settings */
	
	/* Check user input if it is not leave offset as 0. */
	tlc->offset = atoi(argv[1]);

	*target_config = tlc;    

	dmv->dev_type = DM_LINEAR_DEV;
	
	return 0;
}

/*
 * Status routine is called to get params string, which is target
 * specific. When dm_table_status_ioctl is called with flag
 * DM_STATUS_TABLE_FLAG I have to sent params string back.
 */
char *
dm_target_linear_status(void *target_config)
{
	struct target_linear_config *tlc;
	char *params;
	uint32_t i;
	uint32_t count;
	size_t prm_len;

	
	tlc = target_config;    
	prm_len = 0;
	count = 0;

	/* count number of chars in offset */
	for(i = tlc->offset; i != 0; i /= 10)
		count++;
	
	printf("Linear target status function called\n");

	/* length of name + count of chars + one space and null char */
	prm_len = strlen(tlc->pdev->name) + count + 2;

	if ((params = kmem_alloc(prm_len, KM_NOSLEEP)) == NULL)
		return NULL;

	printf("%s %"PRIu64, tlc->pdev->name, tlc->offset);
	snprintf(params, prm_len,"%s %"PRIu64, tlc->pdev->name, tlc->offset);
	
	
	return params;
}

/*
 * Do IO operation, called from dmstrategy routine.
 */
int
dm_target_linear_strategy(struct dm_table_entry *table_en, struct buf *bp)
{
	struct target_linear_config *tlc;

	tlc = table_en->target_config;
	
	printf("Linear target read function called %" PRIu64 "!!\n",
	    tlc->offset);

	bp->b_blkno += tlc->offset;
	
	VOP_STRATEGY(tlc->pdev->pdev_vnode, bp);
	
	return 0;

}

/*
 * Destroy target specific data. Decrement table pdevs.
 */
int
dm_target_linear_destroy(struct dm_table_entry *table_en)
{
	struct target_linear_config *tlc;

	/*
	 * Destroy function is called for every target even if it
	 * doesn't have target_config.
	 */
	
	if (table_en->target_config == NULL)
		return 0;
	
	tlc = table_en->target_config;
	
	/* Decrement device list reference counter */
	tlc->pdev->list_ref_cnt--;
	
	/* If there is no other table which reference this pdev remove it. */
	if (tlc->pdev->list_ref_cnt == 0)
		SLIST_REMOVE(&table_en->dm_dev->pdevs, tlc->pdev, dm_pdev, next_pdev);
			
	/* Decrement pdev ref counter if 0 remove it */
	dm_pdev_decr(tlc->pdev);
	
	kmem_free(table_en->target_config, sizeof(struct target_linear_config));

	table_en->target_config = NULL;
	
	return 0;
}

/*
 * Register upcall device.
 * Linear target doesn't need any upcall devices but other targets like
 * mirror, snapshot, multipath, stripe will use this functionality.
 */
int
dm_target_linear_upcall(struct dm_table_entry *table_en, struct buf *bp)
{

	return 0;
}

/*
 * Transform char s to uint64_t offset number.
 */
uint64_t
atoi(const char *s)
{
	uint64_t n;

	n = 0;

	while (*s != '\0') {

		if (!isdigit(*s))
			break;

		n = (10 * n) + (*s - '0');
		s++;
	}

	return n;
}
