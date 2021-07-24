/*        $NetBSD: dm_target_flakey.c,v 1.4 2021/07/24 21:31:37 andvar Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: dm_target_flakey.c,v 1.4 2021/07/24 21:31:37 andvar Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/kmem.h>

#include "dm.h"

//#define DEBUG_FLAKEY
//#define HAS_BUF_PRIV2 /* XXX requires nonexistent buf::b_private2. */

typedef struct target_flakey_config {
	dm_pdev_t *pdev;
	uint64_t offset;
	int up_int;
	int down_int;
	int offset_time; /* XXX "tick" in hz(9) not working. */

	/* drop_writes feature */
	int drop_writes;

	/* corrupt_bio_byte feature */
	unsigned int corrupt_buf_byte;
	unsigned int corrupt_buf_rw;
	unsigned int corrupt_buf_value;
	unsigned int corrupt_buf_flags; /* for B_XXX flags */
} dm_target_flakey_config_t;

#define BUF_CMD_READ	1
#define BUF_CMD_WRITE	2

#define FLAKEY_CORRUPT_DIR(tfc) \
	((tfc)->corrupt_buf_rw == BUF_CMD_READ ? 'r' : 'w')

static int _init_features(dm_target_flakey_config_t*, int, char**);
static __inline void _submit(dm_target_flakey_config_t*, struct buf*);
static int _flakey_read(dm_target_flakey_config_t*, struct buf*);
static int _flakey_write(dm_target_flakey_config_t*, struct buf*);
static int _flakey_corrupt_buf(dm_target_flakey_config_t*, struct buf*);

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

MODULE(MODULE_CLASS_MISC, dm_target_flakey, NULL);

static int
dm_target_flakey_modcmd(modcmd_t cmd, void *arg)
{
	dm_target_t *dmt;
	int r;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if ((dmt = dm_target_lookup("flakey")) != NULL) {
			dm_target_unbusy(dmt);
			return EEXIST;
		}
		dmt = dm_target_alloc("flakey");

		dmt->version[0] = 1;
		dmt->version[1] = 0;
		dmt->version[2] = 0;
		dmt->init = &dm_target_flakey_init;
		dmt->table = &dm_target_flakey_table;
		dmt->strategy = &dm_target_flakey_strategy;
		dmt->sync = &dm_target_flakey_sync;
		dmt->destroy = &dm_target_flakey_destroy;
		//dmt->upcall = &dm_target_flakey_upcall;
		dmt->secsize = &dm_target_flakey_secsize;

		r = dm_target_insert(dmt);

		break;

	case MODULE_CMD_FINI:
		r = dm_target_rem("flakey");
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
dm_target_flakey_init(dm_table_entry_t *table_en, int argc, char **argv)
{
	dm_target_flakey_config_t *tfc;
	dm_pdev_t *dmp;
	int err;

	if (argc < 4) {
		printf("Flakey target takes at least 4 args, %d given\n", argc);
		return EINVAL;
	}

	aprint_debug("Flakey target init function called: argc=%d\n", argc);

	/* Insert dmp to global pdev list */
	if ((dmp = dm_pdev_insert(argv[0])) == NULL)
		return ENOENT;

	tfc = kmem_alloc(sizeof(dm_target_flakey_config_t), KM_SLEEP);
	tfc->pdev = dmp;
	tfc->offset = atoi64(argv[1]);
	tfc->up_int = atoi64(argv[2]);
	tfc->down_int = atoi64(argv[3]);
	tfc->offset_time = tick;

	if ((tfc->up_int + tfc->down_int) == 0) {
		printf("Sum of up/down interval is 0\n");
		err = EINVAL;
		goto fail;
	}

	if (tfc->up_int + tfc->down_int < tfc->up_int) {
		printf("Interval time overflow\n");
		err = EINVAL;
		goto fail;
	}

	err = _init_features(tfc, argc - 4, argv + 4);
	if (err)
		goto fail;

	dm_table_add_deps(table_en, dmp);
	table_en->target_config = tfc;

	return 0;
fail:
	kmem_free(tfc, sizeof(*tfc));
	return err;
}

static int
_init_features(dm_target_flakey_config_t *tfc, int argc, char **argv)
{
	char *arg;
	unsigned int value;

	if (argc == 0)
		return 0;

	argc = atoi64(*argv++); /* # of args for features */
	if (argc > 6) {
		printf("Invalid # of feature args %d\n", argc);
		return EINVAL;
	}

	while (argc) {
		argc--;
		arg = *argv++;

		/* drop_writes */
		if (strcmp(arg, "drop_writes") == 0) {
			tfc->drop_writes = 1;
			continue;
		}

		/* corrupt_bio_byte <Nth_byte> <direction> <value> <flags> */
		if (strcmp(arg, "corrupt_bio_byte") == 0) {
			if (argc < 4) {
				printf("Invalid # of feature args %d for "
				    "corrupt_bio_byte\n", argc);
				return EINVAL;
			}

			/* <Nth_byte> */
			argc--;
			value = atoi64(*argv++);
			if (value < 1) {
				printf("Invalid corrupt_bio_byte "
				    "<Nth_byte> arg %u\n", value);
				return EINVAL;
			}
			tfc->corrupt_buf_byte = value;

			/* <direction> */
			argc--;
			arg = *argv++;
			if (strcmp(arg, "r") == 0) {
				tfc->corrupt_buf_rw = BUF_CMD_READ;
			} else if (strcmp(arg, "w") == 0) {
				tfc->corrupt_buf_rw = BUF_CMD_WRITE;
			} else {
				printf("Invalid corrupt_bio_byte "
				    "<direction> arg %s\n", arg);
				return EINVAL;
			}

			/* <value> */
			argc--;
			value = atoi64(*argv++);
			if (value > 0xff) {
				printf("Invalid corrupt_bio_byte "
				    "<value> arg %u\n", value);
				return EINVAL;
			}
			tfc->corrupt_buf_value = value;

			/* <flags> */
			argc--;
			tfc->corrupt_buf_flags = atoi64(*argv++);

			continue;
		}

		printf("Unknown Flakey target feature %s\n", arg);
		return EINVAL;
	}

	if (tfc->drop_writes && (tfc->corrupt_buf_rw == BUF_CMD_WRITE)) {
		printf("Flakey target doesn't allow drop_writes feature and "
		    "corrupt_bio_byte feature with 'w' set\n");
		return EINVAL;
	}

	return 0;
}

char *
dm_target_flakey_table(void *target_config)
{
	dm_target_flakey_config_t *tfc;
	char *params, *p;
	int drop_writes;

	tfc = target_config;
	KASSERT(tfc != NULL);

	aprint_debug("Flakey target table function called\n");

	drop_writes = tfc->drop_writes;

	params = kmem_alloc(DM_MAX_PARAMS_SIZE, KM_SLEEP);
	p = params;
	p += snprintf(p, DM_MAX_PARAMS_SIZE, "%s %d %d %d %u ",
	    tfc->pdev->udev_name, tfc->offset_time,
	    tfc->up_int, tfc->down_int,
	    drop_writes + (tfc->corrupt_buf_byte > 0) * 5);

	if (drop_writes)
		p += snprintf(p, DM_MAX_PARAMS_SIZE, "drop_writes ");

	if (tfc->corrupt_buf_byte)
		p += snprintf(p, DM_MAX_PARAMS_SIZE,
		    "corrupt_bio_byte %u %c %u %u ",
		    tfc->corrupt_buf_byte,
		    FLAKEY_CORRUPT_DIR(tfc),
		    tfc->corrupt_buf_value,
		    tfc->corrupt_buf_flags);
	*(--p) = '\0';

	return params;
}

#ifdef DEBUG_FLAKEY
static int count = 0;
#endif

int
dm_target_flakey_strategy(dm_table_entry_t *table_en, struct buf *bp)
{
	dm_target_flakey_config_t *tfc;
#ifndef DEBUG_FLAKEY
	int elapsed;
#endif

	tfc = table_en->target_config;
#ifndef DEBUG_FLAKEY
	elapsed = (tick - tfc->offset_time) / hz;
	if (elapsed % (tfc->up_int + tfc->down_int) >= tfc->up_int) {
#else
	if (++count % 100 == 0) {
#endif
		if (bp->b_flags & B_READ)
			return _flakey_read(tfc, bp);
		else
			return _flakey_write(tfc, bp);
	}

	/* This is what linear target does */
	_submit(tfc, bp);

	return 0;
}

static __inline void
_submit(dm_target_flakey_config_t *tfc, struct buf *bp)
{

	bp->b_blkno += tfc->offset;
	VOP_STRATEGY(tfc->pdev->pdev_vnode, bp);
}

static __inline void
_flakey_eio_buf(struct buf *bp)
{

	bp->b_error = EIO;
	bp->b_resid = 0;
}

static void
_flakey_nestiobuf_iodone(buf_t *bp)
{
#ifdef HAS_BUF_PRIV2
	dm_target_flakey_config_t *tfc;
#endif
	buf_t *mbp = bp->b_private;
	int error;
	int donebytes;

	KASSERT(bp->b_bcount <= bp->b_bufsize);
	KASSERT(mbp != bp);

	error = bp->b_error;
	if (bp->b_error == 0 &&
	    (bp->b_bcount < bp->b_bufsize || bp->b_resid > 0)) {
		/*
		 * Not all got transferred, raise an error. We have no way to
		 * propagate these conditions to mbp.
		 */
		error = EIO;
	}

#ifdef HAS_BUF_PRIV2
	tfc = bp->b_private2;
	/*
	 * Linux dm-flakey has changed its read behavior in 2016.
	 * This conditional is to sync with that change.
	 */
	if (tfc->corrupt_buf_byte && tfc->corrupt_buf_rw == BUF_CMD_READ)
		_flakey_corrupt_buf(tfc, mbp);
	else if (!tfc->drop_writes)
		_flakey_eio_buf(mbp);
#endif
	donebytes = bp->b_bufsize;
	putiobuf(bp);
	nestiobuf_done(mbp, donebytes, error);
}

static int
_flakey_read(dm_target_flakey_config_t *tfc, struct buf *bp)
{
	struct buf *nestbuf;

	/*
	 * Linux dm-flakey has changed its read behavior in 2016.
	 * This conditional is to sync with that change.
	 */
	if (!tfc->corrupt_buf_byte && !tfc->drop_writes) {
		_flakey_eio_buf(bp);
		biodone(bp);
		return 0;
	}

	nestbuf = getiobuf(NULL, true);
	nestiobuf_setup(bp, nestbuf, 0, bp->b_bcount);
	nestbuf->b_iodone = _flakey_nestiobuf_iodone;
	nestbuf->b_blkno = bp->b_blkno;
#ifdef HAS_BUF_PRIV2
	nestbuf->b_private2 = tfc;
#endif
	_submit(tfc, nestbuf);

	return 0;
}

static int
_flakey_write(dm_target_flakey_config_t *tfc, struct buf *bp)
{

	if (tfc->drop_writes) {
		aprint_debug("bp=%p drop_writes blkno=%ju\n", bp, bp->b_blkno);
		biodone(bp);
		return 0;
	}

	if (tfc->corrupt_buf_byte && tfc->corrupt_buf_rw == BUF_CMD_WRITE) {
		_flakey_corrupt_buf(tfc, bp);
		_submit(tfc, bp);
		return 0;
	}

	/* Error all I/Os if neither of the above two */
	_flakey_eio_buf(bp);
	biodone(bp);

	return 0;
}

static int
_flakey_corrupt_buf(dm_target_flakey_config_t *tfc, struct buf *bp)
{
	char *buf;

	if (bp->b_data == NULL)
		return 1;
	if (bp->b_error)
		return 1; /* Don't corrupt on error */
	if (bp->b_bcount < tfc->corrupt_buf_byte)
		return 1;
	if ((bp->b_flags & tfc->corrupt_buf_flags) != tfc->corrupt_buf_flags)
		return 1;

	buf = bp->b_data;
	buf[tfc->corrupt_buf_byte - 1] = tfc->corrupt_buf_value;

	aprint_debug("bp=%p dir=%c blkno=%ju Nth=%u value=%u\n",
	    bp, FLAKEY_CORRUPT_DIR(tfc), bp->b_blkno, tfc->corrupt_buf_byte,
	    tfc->corrupt_buf_value);

	return 0;
}

int
dm_target_flakey_sync(dm_table_entry_t *table_en)
{
	dm_target_flakey_config_t *tfc;
	int cmd;

	tfc = table_en->target_config;
	cmd = 1;

	return VOP_IOCTL(tfc->pdev->pdev_vnode, DIOCCACHESYNC, &cmd,
	    FREAD | FWRITE, kauth_cred_get());
}

int
dm_target_flakey_destroy(dm_table_entry_t *table_en)
{

	if (table_en->target_config == NULL)
		goto out;

	dm_target_flakey_config_t *tfc = table_en->target_config;

	/* Decrement pdev ref counter if 0 remove it */
	dm_pdev_decr(tfc->pdev);

	kmem_free(tfc, sizeof(*tfc));
out:
	/* Unbusy target so we can unload it */
	dm_target_unbusy(table_en->target);

	return 0;
}

#if 0
int
dm_target_flakey_upcall(dm_table_entry_t *table_en, struct buf *bp)
{

	return 0;
}
#endif

int
dm_target_flakey_secsize(dm_table_entry_t *table_en, unsigned int *secsizep)
{
	dm_target_flakey_config_t *tfc;
	unsigned int secsize;

	secsize = 0;

	tfc = table_en->target_config;
	if (tfc != NULL)
		secsize = tfc->pdev->pdev_secsize;

	*secsizep = secsize;

	return 0;
}
