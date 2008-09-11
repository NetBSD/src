/*        $NetBSD: dm_pdev.c,v 1.1.2.10 2008/09/11 13:40:47 haad Exp $      */

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
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/namei.h>
#include <sys/vnode.h>

#include <dev/dkvar.h>

#include "dm.h"

static struct dm_pdev *dm_pdev_alloc(const char *);
static int dm_pdev_rem(struct dm_pdev *);
/*static int dm_pdev_dump_list(void);*/

/*
 * Find used pdev with name == dm_pdev_name.
 */
struct dm_pdev*
dm_pdev_lookup_name(const char *dm_pdev_name)
{
	struct dm_pdev *dm_pdev;
	int dlen; int slen;

	slen = strlen(dm_pdev_name);

	SLIST_FOREACH(dm_pdev, &dm_pdev_list, next_pdev) {
		dlen = strlen(dm_pdev->name);

		if (slen != dlen)
			continue;
		
		if (strncmp(dm_pdev_name, dm_pdev->name, slen) == 0)
			return dm_pdev;
	}

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
		dmp->ref_cnt++;
		aprint_verbose("dmp_pdev_insert pdev %s already in tree\n",dev_name);
		return dmp;
	}

	if ((dmp = dm_pdev_alloc(dev_name)) == NULL)
		return NULL;

	error = dk_lookup(dev_name, curlwp, &dmp->pdev_vnode, UIO_SYSSPACE);

	if (error) {
		aprint_verbose("dk_lookup on device: %s failed with error %d!\n",
		    dev_name, error);

		kmem_free(dmp, sizeof(struct dm_pdev));
		
		return NULL;
	}
	
	dmp->ref_cnt = 1;

	SLIST_INSERT_HEAD(&dm_pdev_list, dmp, next_pdev);

	return dmp;
}

/*
 * Initialize pdev subsystem.
 */
int
dm_pdev_init(void)
{
	
	SLIST_INIT(&dm_pdev_list); /* initialize global pdev list */

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

	if ((dmp = kmem_zalloc(sizeof(struct dm_pdev), KM_NOSLEEP)) == NULL)
		return NULL;

	strlcpy(dmp->name, name, MAX_DEV_NAME);
	
	dmp->ref_cnt = 0;
	dmp->pdev_vnode = NULL;
	
	return dmp;
}

/*
 * Destroy allocated dm_pdev.
 */
static int
dm_pdev_rem(struct dm_pdev *dmp)
{
	int err;

	if (dmp == NULL)
		return ENOENT;
		
	if (dmp->pdev_vnode != NULL) {
		err = vn_close(dmp->pdev_vnode, FREAD | FWRITE, FSCRED);
		if (err != 0)
			return err;
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

	while (!SLIST_EMPTY(&dm_pdev_list)) {           /* List Deletion. */
		
		dm_pdev = SLIST_FIRST(&dm_pdev_list);
		
		SLIST_REMOVE_HEAD(&dm_pdev_list, next_pdev);

		dm_pdev_rem(dm_pdev);
	}

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
 * Decrement pdev reference counter if 0 remove it.
 */
int
dm_pdev_decr(struct dm_pdev *dmp)
{
	if (dmp == NULL)
		return ENOENT;
	
	dmp->ref_cnt--;

	/*
	 * If this was last reference remove dmp from
	 * global list also.
	 */
	if (dmp->ref_cnt == 0) {
		SLIST_REMOVE(&dm_pdev_list, dmp, dm_pdev, next_pdev); 
		dm_pdev_rem(dmp);
	}

	return 0;
}

/*static int
  dm_pdev_dump_list()
  {
  struct dm_pdev *dmp;
	
  aprint_verbose("Dumping dm_pdev_list \n");
	
  SLIST_FOREACH(dmp, &dm_pdev_list, next_pdev) {
  aprint_verbose("dm_pdev_name %s ref_cnt %d list_rf_cnt %d\n",
  dmp->name, dmp->ref_cnt, dmp->list_ref_cnt);
  }
	
  return 0;	
	
  }*/
