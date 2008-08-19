/*        $NetBSD: dm_pdev.c,v 1.1.2.5 2008/08/19 23:42:11 haad Exp $      */

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

#include <sys/types.h>
#include <sys/param.h>

#include <sys/disk.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <sys/namei.h>
#include <sys/vnode.h>

#include <dev/dkvar.h>

#include "dm.h"

static struct dm_pdev *dm_pdev_alloc(const char *);
static int dm_pdev_rem(struct dm_pdev *);

/*
 * XXX. I should use rwlock here.
 */
kmutex_t dm_pdev_mutex;

/*
 * Find used pdev with name == dm_pdev_name.
 */
struct dm_pdev*
dm_pdev_lookup_name(const char *dm_pdev_name)
{
	struct dm_pdev *dm_pdev;
	int dlen; int slen;

	slen = strlen(dm_pdev_name);

	mutex_enter(&dm_pdev_mutex);

	SLIST_FOREACH(dm_pdev, &dm_pdev_list, next_pdev) {
		dlen = strlen(dm_pdev->name);

		if (slen != dlen)
			continue;
		
		if (strncmp(dm_pdev_name, dm_pdev->name, slen) == 0){
			mutex_exit(&dm_pdev_mutex);
			return dm_pdev;
		}
	}
	mutex_exit(&dm_pdev_mutex);

	return NULL;
}

/*
 * Create entry for device with name dev_name and open vnode for it.
 * If entry already exists in global SLIST I will only increment
 * reference counter.
 */
struct dm_pdev*
dm_pdev_insert(const char *dev_name)
{
	struct dm_pdev *dmp;
	int error;

	dmp = dm_pdev_lookup_name(dev_name);

	
	if (dmp != NULL) {

		mutex_enter(&dm_pdev_mutex);
		
		dmp->ref_cnt++;

		mutex_exit(&dm_pdev_mutex);
		
		return dmp;
	}

	if ((dmp = dm_pdev_alloc(dev_name)) == NULL)
		return NULL;

	error = dk_lookup(dev_name, curlwp, &dmp->pdev_vnode, UIO_SYSSPACE);

	if (error) {
		aprint_normal("dk_lookup on device: %s failed with error %d!\n",
		    dev_name, error);

		return NULL;
	}
	
	dmp->ref_cnt = 1;
	
	mutex_enter(&dm_pdev_mutex);

	SLIST_INSERT_HEAD(&dm_pdev_list, dmp, next_pdev);
	
	mutex_exit(&dm_pdev_mutex);

	return dmp;
}

/*
 * Initialize pdev subsystem.
 */
int
dm_pdev_init(void)
{
	
	SLIST_INIT(&dm_pdev_list); /* initialize global pdev list */
	
	mutex_init(&dm_pdev_mutex,MUTEX_DEFAULT,IPL_NONE);

	return 0;
}

/*
 * Allocat new pdev structure if is not already present and
 * set name.
 */
static struct dm_pdev*
dm_pdev_alloc(const char *name)
{
	struct dm_pdev *dmp;

	if ((dmp = kmem_alloc(sizeof(struct dm_pdev)+strlen(name)+1,
		    KM_NOSLEEP)) == NULL)
		return NULL;

	strlcpy(dmp->name, name, MAX_DEV_NAME);
	
	dmp->ref_cnt = 0;
	dmp->pdev_vnode = NULL;
	
	return dmp;
}

/*
 * Destroy allocated dm_pdev. This routine is called with held pdev mutex.
 */
static int
dm_pdev_rem(struct dm_pdev *dmp)
{
	if (dmp == NULL)
		return ENOENT;

	if (dmp->pdev_vnode != NULL) {
		VOP_CLOSE(dmp->pdev_vnode, FREAD|FWRITE, FSCRED);
		vput(dmp->pdev_vnode);
	}

	kmem_free(dmp, sizeof(*dmp));

	dmp = NULL;
	
	return 0;
}

/*
 * Destroy all existing pdev's in device-mapper.
 */
int
dm_pdev_destroy(void)
{
	struct dm_pdev *dm_pdev;
	
	mutex_enter(&dm_pdev_mutex);

	while (!SLIST_EMPTY(&dm_pdev_list)) {           /* List Deletion. */
		
		dm_pdev = SLIST_FIRST(&dm_pdev_list);
		
		SLIST_REMOVE_HEAD(&dm_pdev_list, next_pdev);

		dm_pdev_rem(dm_pdev);
	}
	
	mutex_exit(&dm_pdev_mutex);

	return 0;
}

/*
 * This funcion is called from dm_dev_remove_ioctl.
 * When I'm removing device from list, I have to decrement
 * reference counter. If reference counter is 0 I will remove
 * dmp from global list and from device list to. And I will CLOSE
 * dmp vnode too.
 */

/*
 * I'm guarding only global list with mutex, device should be
 * locked elsewhere.
 */
int
dm_pdev_decr(struct dm_pdev *dmp)
{
	if (dmp == NULL)
		return ENOENT;

	mutex_enter(&dm_pdev_mutex);
	
	dmp->ref_cnt--;
	
	/*
	 * If this was last reference remove dmp from
	 * global list also.
	 */
	if (dmp->ref_cnt == 0) {
		SLIST_REMOVE(&dm_pdev_list, dmp, dm_pdev, next_pdev); 
		
		dm_pdev_rem(dmp);
	}

	mutex_exit(&dm_pdev_mutex);

	return 0;
}
