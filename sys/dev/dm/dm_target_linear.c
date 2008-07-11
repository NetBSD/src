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

#include "dm.h"

static uint64_t atoi(const char *);

/*
 * Allocate target specific config data, and link them to table.
 * argv[0] is name,
 * argv[1] is physical data offset.
 */
int
dm_target_linear_init(struct dm_dev *dmv, void **target_config, 
		int argc, const char **argv)
{
	struct target_linear_config *tlc;
	struct dm_pdev *dmp;
		
	printf("Linear target init function called %s--%s!!\n",
	    argv[0], argv[1]);
	
	dmp = dm_pdev_lookup_name(argv[0]);
		
	if ((tlc = kmem_alloc(sizeof(struct target_linear_config), KM_NOSLEEP))
	    == NULL)
		return 1;

	/* XXX howto convert char* to number ? tlc->offset = atoi(argv[1]); */

	tlc->pdev = dmp;
	tlc->offset = 0; 	/* default settings */
	
	/* Check user input if it is not leave offset as 0. */
	if (isdigit((int)argv[1]))
		tlc->offset = atoi(argv[1]);

	*target_config = tlc;    
	
	return 0;
}

/*
 * Do IO operation, called from dmstrategy routine.
 */
int
dm_target_linear_strategy(struct dm_table_entry *table_en, struct buf *bp)
{
	struct target_linear_config *tlc;

	tlc = table_en->target_config;
	
	printf("Linear target read function called!!\n");

	bp->b_blkno += tlc->offset;
	
	VOP_STRATEGY(tlc->pdev->pdev_vnode, bp);
	
	return 0;

}

/*
 * Destroy target specific data.
 */
int
dm_target_linear_destroy(struct dm_table_entry *table_en)
{

	kmem_free(table_en->target_config, sizeof(struct target_linear_config));
	
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
static uint64_t
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
