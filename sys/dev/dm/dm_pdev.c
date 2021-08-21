/*        $NetBSD: dm_pdev.c,v 1.23 2021/08/21 22:23:33 andvar Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: dm_pdev.c,v 1.23 2021/08/21 22:23:33 andvar Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/disk.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/namei.h>

#include <dev/dkvar.h>

#include "dm.h"

static SLIST_HEAD(dm_pdevs, dm_pdev) dm_pdev_list;

static kmutex_t dm_pdev_mutex;

static dm_pdev_t *dm_pdev_alloc(const char *);
static int dm_pdev_rem(dm_pdev_t *);
static dm_pdev_t *dm_pdev_lookup_name(const char *);

/*
* Find used pdev with name == dm_pdev_name.
*/
dm_pdev_t *
dm_pdev_lookup_name(const char *dm_pdev_name)
{
	dm_pdev_t *dm_pdev;
	size_t dlen;
	size_t slen;

	KASSERT(dm_pdev_name != NULL);

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
dm_pdev_t *
dm_pdev_insert(const char *dev_name)
{
	struct pathbuf *dev_pb;
	dm_pdev_t *dmp;
	int error;

	KASSERT(dev_name != NULL);

	mutex_enter(&dm_pdev_mutex);
	dmp = dm_pdev_lookup_name(dev_name);

	if (dmp != NULL) {
		dmp->ref_cnt++;
		aprint_debug("%s: pdev %s already in tree\n",
		    __func__, dev_name);
		mutex_exit(&dm_pdev_mutex);
		return dmp;
	}

	if ((dmp = dm_pdev_alloc(dev_name)) == NULL) {
		mutex_exit(&dm_pdev_mutex);
		return NULL;
	}

	dev_pb = pathbuf_create(dev_name);
	if (dev_pb == NULL) {
		aprint_debug("%s: pathbuf_create on device: %s failed!\n",
		    __func__, dev_name);
		mutex_exit(&dm_pdev_mutex);
		kmem_free(dmp, sizeof(dm_pdev_t));
		return NULL;
	}
	error = vn_bdev_openpath(dev_pb, &dmp->pdev_vnode, curlwp);
	pathbuf_destroy(dev_pb);
	if (error) {
		aprint_debug("%s: lookup on device: %s (error %d)\n",
		    __func__, dev_name, error);
		mutex_exit(&dm_pdev_mutex);
		kmem_free(dmp, sizeof(dm_pdev_t));
		return NULL;
	}
	getdisksize(dmp->pdev_vnode, &dmp->pdev_numsec, &dmp->pdev_secsize);
	dmp->ref_cnt = 1;

	snprintf(dmp->udev_name, sizeof(dmp->udev_name), "%d:%d",
	    major(dmp->pdev_vnode->v_rdev), minor(dmp->pdev_vnode->v_rdev));
	aprint_debug("%s: %s %s\n", __func__, dev_name, dmp->udev_name);

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

	SLIST_INIT(&dm_pdev_list);	/* initialize global pdev list */
	mutex_init(&dm_pdev_mutex, MUTEX_DEFAULT, IPL_NONE);

	return 0;
}

/*
 * Allocat new pdev structure if is not already present and
 * set name.
 */
static dm_pdev_t *
dm_pdev_alloc(const char *name)
{
	dm_pdev_t *dmp;

	dmp = kmem_zalloc(sizeof(*dmp), KM_SLEEP);
	strlcpy(dmp->name, name, sizeof(dmp->name));
	dmp->ref_cnt = 0;
	dmp->pdev_vnode = NULL;

	return dmp;
}

/*
 * Destroy allocated dm_pdev.
 */
static int
dm_pdev_rem(dm_pdev_t *dmp)
{

	KASSERT(dmp != NULL);

	if (dmp->pdev_vnode != NULL) {
		int error = vn_close(dmp->pdev_vnode, FREAD | FWRITE, FSCRED);
		if (error != 0) {
			kmem_free(dmp, sizeof(*dmp));
			return error;
		}
	}
	kmem_free(dmp, sizeof(*dmp));

	return 0;
}

/*
 * Destroy all existing pdev's in device-mapper.
 */
int
dm_pdev_destroy(void)
{
	dm_pdev_t *dmp;

	mutex_enter(&dm_pdev_mutex);

	while ((dmp = SLIST_FIRST(&dm_pdev_list)) != NULL) {
		SLIST_REMOVE(&dm_pdev_list, dmp, dm_pdev, next_pdev);
		dm_pdev_rem(dmp);
	}
	KASSERT(SLIST_EMPTY(&dm_pdev_list));

	mutex_exit(&dm_pdev_mutex);

	mutex_destroy(&dm_pdev_mutex);
	return 0;
}

/*
 * This function is called from dm_dev_remove_ioctl.
 * When I'm removing device from list, I have to decrement
 * reference counter. If reference counter is 0 I will remove
 * dmp from global list and from device list to. And I will CLOSE
 * dmp vnode too.
 */

/*
 * Decrement pdev reference counter if 0 remove it.
 */
int
dm_pdev_decr(dm_pdev_t *dmp)
{

	KASSERT(dmp != NULL);
	/*
	 * If this was last reference remove dmp from
	 * global list also.
	 */
	mutex_enter(&dm_pdev_mutex);

	if (--dmp->ref_cnt == 0) {
		SLIST_REMOVE(&dm_pdev_list, dmp, dm_pdev, next_pdev);
		mutex_exit(&dm_pdev_mutex);
		dm_pdev_rem(dmp);
		return 0;
	}

	mutex_exit(&dm_pdev_mutex);
	return 0;
}

#if 0
static int
dm_pdev_dump_list(void)
{
	dm_pdev_t *dmp;

	aprint_verbose("Dumping dm_pdev_list\n");

	SLIST_FOREACH(dmp, &dm_pdev_list, next_pdev) {
		aprint_verbose("dm_pdev_name %s ref_cnt %d list_rf_cnt %d\n",
		dmp->name, dmp->ref_cnt, dmp->list_ref_cnt);
	}

	return 0;
}
#endif
