/*        $NetBSD: dm_target_error.c,v 1.1.2.5 2008/08/19 13:30:36 haad Exp $      */

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
 * This file implements initial version of device-mapper error target.
 */
#include <sys/types.h>
#include <sys/param.h>

#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/kmem.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>

#include "dm.h"

/* Init function called from dm_table_load_ioctl. */
int
dm_target_error_init(struct dm_dev *dmv, void **target_config, char *argv)
{

	printf("Error target init function called!!\n");

	*target_config = NULL;

	dmv->dev_type = DM_ERROR_DEV;
	
	return 0;
}

/* Status routine called to get params string. */
char *
dm_target_error_status(void *target_config)
{
	return NULL;
}	

/* Strategy routine called from dm_strategy. */
int
dm_target_error_strategy(struct dm_table_entry *table_en, struct buf *bp)
{

	printf("Error target read function called!!\n");

	bp->b_error = EIO;
	bp->b_resid = 0;

	biodone(bp);
	
	return 0;
}

/* Doesn't do anything here. */
int
dm_target_error_destroy(struct dm_table_entry *table_en)
{
	table_en->target_config = NULL;

	return 0;
}

/* Unsupported for this target. */
int
dm_target_error_upcall(struct dm_table_entry *table_en, struct buf *bp)
{
	return 0;
}
