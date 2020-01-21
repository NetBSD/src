/*        $NetBSD: dm_target_delay.c,v 1.2 2020/01/21 16:27:53 tkusumi Exp $      */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * Copyright (c) 2015 The DragonFly Project.  All rights reserved.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <tkusumi@netbsd.org>.
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
__KERNEL_RCSID(0, "$NetBSD: dm_target_delay.c,v 1.2 2020/01/21 16:27:53 tkusumi Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/callout.h>
#include <sys/pool.h>
#include <sys/kthread.h>

#include "dm.h"

struct dm_delay_buf {
	TAILQ_ENTRY(dm_delay_buf) entry;
	struct buf *bp;
	int expire;
};
TAILQ_HEAD(dm_delay_buf_list, dm_delay_buf);

struct dm_delay_info {
	dm_pdev_t *pdev;
	uint64_t offset;
	int delay;
	int count;
	int enabled;
	struct dm_delay_buf_list buf_list;
	struct callout cal;
	kmutex_t buf_mtx;
	kmutex_t cal_mtx;
	kmutex_t token; /* lwkt_token in DragonFly */
};

typedef struct target_delay_config {
	struct dm_delay_info read;
	struct dm_delay_info write;
	int argc; /* either 3 or 6 */
} dm_target_delay_config_t;

static int _init(struct dm_delay_info*, char**);
static int _table(struct dm_delay_info*, char*);
static void _strategy(struct dm_delay_info*, struct buf*);
static __inline void _submit(struct dm_delay_info*, struct buf*);
static void _destroy(struct dm_delay_info*);
static void _timeout(void*);
static void _thread(void*);
static __inline void _debug(struct dm_delay_info*, const char*);

static struct pool pool;
static bool pool_initialized = false;

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

MODULE(MODULE_CLASS_MISC, dm_target_delay, NULL);

static int
dm_target_delay_modcmd(modcmd_t cmd, void *arg)
{
	dm_target_t *dmt;
	int r;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if ((dmt = dm_target_lookup("delay")) != NULL) {
			dm_target_unbusy(dmt);
			return EEXIST;
		}
		dmt = dm_target_alloc("delay");

		dmt->version[0] = 1;
		dmt->version[1] = 0;
		dmt->version[2] = 0;
		dmt->init = &dm_target_delay_init;
		dmt->table = &dm_target_delay_table;
		dmt->strategy = &dm_target_delay_strategy;
		dmt->sync = &dm_target_delay_sync;
		dmt->destroy = &dm_target_delay_destroy;
		//dmt->upcall = &dm_target_delay_upcall;
		dmt->secsize = &dm_target_delay_secsize;

		dm_target_delay_pool_create();
		r = dm_target_insert(dmt);

		break;

	case MODULE_CMD_FINI:
		r = dm_target_rem("delay");
		dm_target_delay_pool_destroy();
		break;

	case MODULE_CMD_STAT:
		return ENOTTY;

	default:
		return ENOTTY;
	}

	return r;
}
#endif

int
dm_target_delay_init(dm_table_entry_t *table_en, int argc, char **argv)
{
	dm_target_delay_config_t *tdc;
	int ret;

	if (argc != 3 && argc != 6) {
		printf("Delay target takes 3 or 6 args, %d given\n", argc);
		return EINVAL;
	}

	aprint_debug("Delay target init function called: argc=%d\n", argc);

	tdc = kmem_alloc(sizeof(dm_target_delay_config_t), KM_SLEEP);
	tdc->argc = argc;

	ret = _init(&tdc->read, argv);
	if (ret) {
		kmem_free(tdc, sizeof(*tdc));
		return ret;
	}

	if (argc == 6)
		argv += 3;

	ret = _init(&tdc->write, argv);
	if (ret) {
		dm_pdev_decr(tdc->read.pdev);
		kmem_free(tdc, sizeof(*tdc));
		return ret;
	}

	dm_table_add_deps(table_en, tdc->read.pdev);
	dm_table_add_deps(table_en, tdc->write.pdev);

	table_en->target_config = tdc;

	return 0;
}

static int
_init(struct dm_delay_info *di, char **argv)
{
	dm_pdev_t *dmp;
	int tmp;

	if (argv[0] == NULL)
		return EINVAL;
	if ((dmp = dm_pdev_insert(argv[0])) == NULL)
		return ENOENT;

	di->pdev = dmp;
	di->offset = atoi64(argv[1]);
	tmp = atoi64(argv[2]);
	di->delay = tmp * hz / 1000;
	di->count = 0;

	TAILQ_INIT(&di->buf_list);
	callout_init(&di->cal, 0);
	mutex_init(&di->buf_mtx, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&di->cal_mtx, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&di->token, MUTEX_DEFAULT, IPL_NONE);

	di->enabled = 1;
	if (kthread_create(PRI_NONE, 0, NULL, _thread, di, NULL,
	    "dmdlthread")) {
		printf("Failed to create delay target's kthread\n");
		/* XXX cleanup ? */
		return EINVAL;
	}

	_debug(di, "init");
	return 0;
}

char *
dm_target_delay_info(void *target_config)
{
	dm_target_delay_config_t *tdc;
	char *params;

	tdc = target_config;
	KASSERT(tdc != NULL);

	aprint_debug("Delay target info function called\n");

	params = kmem_alloc(DM_MAX_PARAMS_SIZE, KM_SLEEP);
	snprintf(params, DM_MAX_PARAMS_SIZE, "%d %d", tdc->read.count,
	    tdc->write.count);

	return params;
}

char *
dm_target_delay_table(void *target_config)
{
	dm_target_delay_config_t *tdc;
	char *params, *p;

	tdc = target_config;
	KASSERT(tdc != NULL);

	aprint_debug("Delay target table function called\n");

	params = kmem_alloc(DM_MAX_PARAMS_SIZE, KM_SLEEP);
	p = params;
	p += _table(&tdc->read, p);
	if (tdc->argc == 6) {
		p += snprintf(p, DM_MAX_PARAMS_SIZE, " ");
		_table(&tdc->write, p);
	}

	return params;
}

static int
_table(struct dm_delay_info *di, char *p)
{

	return snprintf(p, DM_MAX_PARAMS_SIZE, "%s %" PRIu64 " %d",
	    di->pdev->udev_name, di->offset, di->delay);
}

int
dm_target_delay_strategy(dm_table_entry_t *table_en, struct buf *bp)
{
	dm_target_delay_config_t *tdc;
	struct dm_delay_info *di;

	tdc = table_en->target_config;
	KASSERT(tdc != NULL);

	if (bp->b_flags & B_READ)
		di = &tdc->read;
	else
		di = &tdc->write;

	if (di) {
		if (di->delay) {
			_strategy(di, bp);
		} else {
			_submit(di, bp);
		}
	} else {
		struct vnode *vnode = tdc->write.pdev->pdev_vnode;
		KASSERT(0);
		VOP_STRATEGY(vnode, bp);
	}

	return 0;
}

static void
_strategy(struct dm_delay_info *di, struct buf *bp)
{
	struct dm_delay_buf *dp;

	dp = pool_get(&pool, PR_WAITOK);
	dp->bp = bp;
	dp->expire = tick + di->delay;

	mutex_enter(&di->buf_mtx);
	di->count++;
	TAILQ_INSERT_TAIL(&di->buf_list, dp, entry);
	mutex_exit(&di->buf_mtx);

	mutex_enter(&di->cal_mtx);
	if (!callout_pending(&di->cal))
		callout_reset(&di->cal, di->delay, _timeout, di);
	mutex_exit(&di->cal_mtx);
}

static __inline void
_submit(struct dm_delay_info *di, struct buf *bp)
{

	_debug(di, "submit");

	bp->b_blkno += di->offset;
	VOP_STRATEGY(di->pdev->pdev_vnode, bp);
}

static void
_submit_queue(struct dm_delay_info *di, int submit_all)
{
	struct dm_delay_buf *dp;
	struct dm_delay_buf_list tmp_list;
	int next = -1;
	int reset = 0;

	_debug(di, "submitq");
	TAILQ_INIT(&tmp_list);

	mutex_enter(&di->buf_mtx);
	while ((dp = TAILQ_FIRST(&di->buf_list)) != NULL) {
		if (submit_all || tick > dp->expire) {
			TAILQ_REMOVE(&di->buf_list, dp, entry);
			TAILQ_INSERT_TAIL(&tmp_list, dp, entry);
			di->count--;
			continue;
		}
		if (reset == 0) {
			reset = 1;
			next = dp->expire;
		} else {
			next = lmin(next, dp->expire);
		}
	}
	mutex_exit(&di->buf_mtx);

	if (reset) {
		mutex_enter(&di->cal_mtx);
		callout_reset(&di->cal, next - tick, _timeout, di);
		mutex_exit(&di->cal_mtx);
	}

	while ((dp = TAILQ_FIRST(&tmp_list)) != NULL) {
		TAILQ_REMOVE(&tmp_list, dp, entry);
		_submit(di, dp->bp);
		pool_put(&pool, dp);
	}
}

int
dm_target_delay_sync(dm_table_entry_t *table_en)
{
	dm_target_delay_config_t *tdc;
	int cmd;

	tdc = table_en->target_config;
	cmd = 1;

	return VOP_IOCTL(tdc->write.pdev->pdev_vnode, DIOCCACHESYNC, &cmd,
	    FREAD | FWRITE, kauth_cred_get());
}

int
dm_target_delay_destroy(dm_table_entry_t *table_en)
{
	dm_target_delay_config_t *tdc;

	tdc = table_en->target_config;
	if (tdc == NULL)
		goto out;

	_destroy(&tdc->read);
	_destroy(&tdc->write);

	kmem_free(tdc, sizeof(*tdc));
out:
	/* Unbusy target so we can unload it */
	dm_target_unbusy(table_en->target);

	return 0;
}

static void
_destroy(struct dm_delay_info *di)
{

	_debug(di, "destroy");

	mutex_enter(&di->token);
	di->enabled = 0;

	mutex_enter(&di->cal_mtx);
	if (callout_pending(&di->cal))
		callout_halt(&di->cal, NULL);
	mutex_exit(&di->cal_mtx);

	_submit_queue(di, 1);
	mutex_exit(&di->token);
	wakeup(di);
	/* XXX race here (DragonFly uses lwkt_token instead of mutex) */
	tsleep(&di->enabled, 0, "dmdldestroy", 0);

	mutex_destroy(&di->token);
	mutex_destroy(&di->cal_mtx);
	mutex_destroy(&di->buf_mtx);

	dm_pdev_decr(di->pdev);
}

#if 0
int
dm_target_delay_upcall(dm_table_entry_t *table_en, struct buf *bp)
{

	return 0;
}
#endif

int
dm_target_delay_secsize(dm_table_entry_t *table_en, unsigned int *secsizep)
{
	dm_target_delay_config_t *tdc;
	unsigned int secsize;

	secsize = 0;

	tdc = table_en->target_config;
	if (tdc != NULL)
		secsize = tdc->write.pdev->pdev_secsize;

	*secsizep = secsize;

	return 0;
}

static void
_timeout(void *arg)
{
	struct dm_delay_info *di = arg;

	_debug(di, "timeout");
	wakeup(di);
}

static void
_thread(void *arg)
{
	struct dm_delay_info *di = arg;

	_debug(di, "thread init");
	mutex_enter(&di->token);

	while (di->enabled) {
		mutex_exit(&di->token);
		tsleep(di, 0, "dmdlthread", 0);
		_submit_queue(di, 0);
		mutex_enter(&di->token);
	}

	mutex_exit(&di->token);
	wakeup(&di->enabled);

	_debug(di, "thread exit");
	kthread_exit(0);
}

static __inline void
_debug(struct dm_delay_info *di, const char *msg)
{

	aprint_debug("%-8s: %d pdev=%s offset=%ju delay=%d count=%d\n",
	    msg, di->enabled, di->pdev->name, (uintmax_t)di->offset, di->delay,
	    di->count);
}

void
dm_target_delay_pool_create(void)
{

	if (!pool_initialized) {
		pool_init(&pool, sizeof(struct dm_delay_buf), 0, 0, 0,
		    "dmdlobj", &pool_allocator_nointr, IPL_NONE);
		pool_initialized = true;
		aprint_debug("Delay target pool initialized\n");
	}
}

void
dm_target_delay_pool_destroy(void)
{

	if (pool_initialized) {
		pool_destroy(&pool);
		pool_initialized = false;
		aprint_debug("Delay target pool destroyed\n");
	}
}
