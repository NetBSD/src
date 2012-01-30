/*	$NetBSD: proplib-interpreter.c,v 1.2 2012/01/30 19:28:11 dholland Exp $	*/

/*
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_vfsops.c	8.8 (Berkeley) 5/20/95
 *	From NetBSD: ufs_vfsops.c,v 1.42 2011/03/24 17:05:46 bouyer Exp
 */

/*
 * Copyright (c) 1982, 1986, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_quota.c	8.5 (Berkeley) 5/20/95
 *	From NetBSD: ufs_quota.c,v 1.70 2011/03/24 17:05:46 bouyer Exp
 */

/*
 *	From NetBSD: vfs_quotactl.c,v 1.36 2012/01/29 07:21:59 dholland Exp
 */

/*
 * Note that both of the copyrights above are moderately spurious;
 * this code should almost certainly have the Copyright 2010 Manuel
 * Bouyer notice and license found in e.g. sys/ufs/ufs/quota2_subr.c.
 * However, they're what was on the files this code was sliced out of.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: proplib-interpreter.c,v 1.2 2012/01/30 19:28:11 dholland Exp $");

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <assert.h>

#include <quota.h>
#include <quota/quotaprop.h>

#include "proplib-interpreter.h"

static int
vfs_quotactl_getversion(struct quotahandle *qh,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	prop_array_t replies;
	prop_dictionary_t data;
	unsigned restrictions;
	int q2version;

	assert(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	assert(prop_object_type(datas) == PROP_TYPE_ARRAY);

	restrictions = quota_getrestrictions(qh);

	/*
	 * Set q2version based on the stat results. Currently there
	 * are two valid values for q2version, 1 and 2, which we pick
	 * based on whether quotacheck is required.
	 */
	if (restrictions & QUOTA_RESTRICT_NEEDSQUOTACHECK) {
		q2version = 1;
	} else {
		q2version = 2;
	}

	data = prop_dictionary_create();
	if (data == NULL) {
		return ENOMEM;
	}

	if (!prop_dictionary_set_int8(data, "version", q2version)) {
		prop_object_release(data);
		return ENOMEM;
	}

	replies = prop_array_create();
	if (replies == NULL) {
		prop_object_release(data);
		return ENOMEM;
	}

	if (!prop_array_add_and_rel(replies, data)) {
		prop_object_release(data);
		prop_object_release(replies);
		return ENOMEM;
	}

	if (!prop_dictionary_set_and_rel(cmddict, "data", replies)) {
		prop_object_release(replies);
		return ENOMEM;
	}

	return 0;
}

static int
vfs_quotactl_quotaon(struct quotahandle *qh,
		     prop_dictionary_t cmddict, int q2type,
		     prop_array_t datas)
{
	prop_dictionary_t data;
	const char *qfile;

	assert(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	assert(prop_object_type(datas) == PROP_TYPE_ARRAY);

	if (prop_array_count(datas) != 1)
		return EINVAL;

	data = prop_array_get(datas, 0);
	if (data == NULL)
		return ENOMEM;
	if (!prop_dictionary_get_cstring_nocopy(data, "quotafile",
	    &qfile))
		return EINVAL;

	/* libquota knows the filename; cannot set it from here */
	(void)qfile;

	if (quota_quotaon(qh, q2type)) {
		return errno;
	}
	return 0;
}

static int
vfs_quotactl_quotaoff(struct quotahandle *qh,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	assert(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	assert(prop_object_type(datas) == PROP_TYPE_ARRAY);

	if (prop_array_count(datas) != 0)
		return EINVAL;

	if (quota_quotaoff(qh, q2type)) {
		return errno;
	}
	return 0;
}

static int
vfs_quotactl_get_addreply(const struct quotakey *qk,
			  const struct quotaval *blocks,
			  const struct quotaval *files,
			  prop_array_t replies)
{
	prop_dictionary_t dict;
	id_t id;
	int defaultq;
	uint64_t *valuesp[QUOTA_NLIMITS];

	/* XXX illegal casts */
	valuesp[QUOTA_LIMIT_BLOCK] = (void *)(intptr_t)&blocks->qv_hardlimit;
	valuesp[QUOTA_LIMIT_FILE] =  (void *)(intptr_t)&files->qv_hardlimit;

	if (qk->qk_id == QUOTA_DEFAULTID) {
		id = 0;
		defaultq = 1;
	} else {
		id = qk->qk_id;
		defaultq = 0;
	}

	dict = quota64toprop(id, defaultq, valuesp,
	    ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
	    ufs_quota_limit_names, QUOTA_NLIMITS);
	if (dict == NULL)
		return ENOMEM;
	if (!prop_array_add_and_rel(replies, dict)) {
		prop_object_release(dict);
		return ENOMEM;
	}

	return 0;
}

static int
vfs_quotactl_get(struct quotahandle *qh,
			prop_dictionary_t cmddict, int idtype,
			prop_array_t datas)
{
	prop_object_iterator_t iter;
	prop_dictionary_t data;
	prop_array_t replies;
	uint32_t id;
	const char *idstr;
	struct quotakey qk;
	struct quotaval blocks, files;
	int error;

	assert(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	assert(prop_object_type(datas) == PROP_TYPE_ARRAY);

	replies = prop_array_create();
	if (replies == NULL) {
		return ENOMEM;
	}

	iter = prop_array_iterator(datas);
	if (iter == NULL) {
		prop_object_release(replies);
		return ENOMEM;
	}

	while ((data = prop_object_iterator_next(iter)) != NULL) {
		qk.qk_idtype = idtype;

		if (!prop_dictionary_get_uint32(data, "id", &id)) {
			if (!prop_dictionary_get_cstring_nocopy(data, "id",
			    &idstr))
				continue;
			if (strcmp(idstr, "default")) {
				error = EINVAL;
				goto fail;
			}
			qk.qk_id = QUOTA_DEFAULTID;
		} else {
			qk.qk_id = id;
		}

		qk.qk_objtype = QUOTA_OBJTYPE_BLOCKS;

		if (quota_get(qh, &qk, &blocks)) {
			if (errno == EPERM) {
				/* XXX does this make sense? */
				continue;
			} else if (errno == ENOENT) {
				/* XXX does *this* make sense? */
				continue;
			} else {
				error = errno;
				goto fail;
			}
		}

		qk.qk_objtype = QUOTA_OBJTYPE_FILES;

		if (quota_get(qh, &qk, &files)) {
			if (errno == EPERM) {
				/* XXX does this make sense? */
				continue;
			} else if (errno == ENOENT) {
				/* XXX does *this* make sense? */
				continue;
			} else {
				error = errno;
				goto fail;
			}
		}

		error = vfs_quotactl_get_addreply(&qk, &blocks, &files,
						  replies);
	}

	prop_object_iterator_release(iter);
	if (!prop_dictionary_set_and_rel(cmddict, "data", replies)) {
		error = ENOMEM;
	} else {
		error = 0;
	}

	return error;

 fail:
	prop_object_iterator_release(iter);
	prop_object_release(replies);
	return error;
}

static int
vfs_quotactl_put_extractinfo(prop_dictionary_t data,
			struct quotaval *blocks, struct quotaval *files)
{
	/*
	 * So, the way proptoquota64 works is that you pass it an
	 * array of pointers to uint64. Each of these pointers is
	 * supposed to point to 5 (UFS_QUOTA_NENTRIES) uint64s. This
	 * array of pointers is the second argument. The third and
	 * forth argument are the names of the five values to extract,
	 * and UFS_QUOTA_NENTRIES. The last two arguments are the
	 * names assocated with the pointers (QUOTATYPE_LDICT_BLOCK,
	 * QUOTADICT_LTYPE_FILE) and the number of pointers. Most of
	 * the existing code was unsafely casting struct quotaval
	 * (formerly struct ufs_quota_entry) to (uint64_t *) and using
	 * that as the block of 5 uint64s. Or worse, pointing to
	 * subregions of that and reducing the number of uint64s to
	 * pull "adjacent" values. Demons fly out of your nose!
	 */

	uint64_t bvals[UFS_QUOTA_NENTRIES];
	uint64_t fvals[UFS_QUOTA_NENTRIES];
	uint64_t *valptrs[QUOTA_NLIMITS];
	int error;

	valptrs[QUOTA_LIMIT_BLOCK] = bvals;
	valptrs[QUOTA_LIMIT_FILE] = fvals;
	error = proptoquota64(data, valptrs,
			      ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
			      ufs_quota_limit_names, QUOTA_NLIMITS);
	if (error) {
		return error;
	}

	/*
	 * There are no symbolic constants for these indexes!
	 */

	blocks->qv_hardlimit = bvals[0];
	blocks->qv_softlimit = bvals[1];
	blocks->qv_usage = bvals[2];
	blocks->qv_expiretime = bvals[3];
	blocks->qv_grace = bvals[4];
	files->qv_hardlimit = fvals[0];
	files->qv_softlimit = fvals[1];
	files->qv_usage = fvals[2];
	files->qv_expiretime = fvals[3];
	files->qv_grace = fvals[4];

	return 0;
}

static int
vfs_quotactl_put(struct quotahandle *qh,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	prop_array_t replies;
	prop_object_iterator_t iter;
	prop_dictionary_t data;
	int defaultq;
	uint32_t id;
	const char *idstr;
	struct quotakey qk;
	struct quotaval blocks, files;
	int error;

	assert(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	assert(prop_object_type(datas) == PROP_TYPE_ARRAY);

	replies = prop_array_create();
	if (replies == NULL)
		return ENOMEM;

	iter = prop_array_iterator(datas);
	if (iter == NULL) {
		prop_object_release(replies);
		return ENOMEM;
	}

	while ((data = prop_object_iterator_next(iter)) != NULL) {

		assert(prop_object_type(data) == PROP_TYPE_DICTIONARY);

		if (!prop_dictionary_get_uint32(data, "id", &id)) {
			if (!prop_dictionary_get_cstring_nocopy(data, "id",
			    &idstr))
				continue;
			if (strcmp(idstr, "default"))
				continue;
			id = 0;
			defaultq = 1;
		} else {
			defaultq = 0;
		}

		error = vfs_quotactl_put_extractinfo(data, &blocks, &files);
		if (error) {
			goto err;
		}

		qk.qk_idtype = q2type;
		qk.qk_id = defaultq ? QUOTA_DEFAULTID : id;
		qk.qk_objtype = QUOTA_OBJTYPE_BLOCKS;

		if (quota_put(qh, &qk, &blocks)) {
			error = errno;
			goto err;
		}

		qk.qk_idtype = q2type;
		qk.qk_id = defaultq ? QUOTA_DEFAULTID : id;
		qk.qk_objtype = QUOTA_OBJTYPE_FILES;

		if (quota_put(qh, &qk, &files)) {
			error = errno;
			goto err;
		}
	}
	prop_object_iterator_release(iter);
	if (!prop_dictionary_set_and_rel(cmddict, "data", replies)) {
		error = ENOMEM;
	} else {
		error = 0;
	}
	return error;
err:
	prop_object_iterator_release(iter);
	prop_object_release(replies);
	return error;
}

static prop_dictionary_t
vfs_quotactl_getall_makereply(const struct quotakey *key)
{
	prop_dictionary_t dict;
	id_t id;
	int defaultq;

	dict = prop_dictionary_create();
	if (dict == NULL)
		return NULL;

	id = key->qk_id;
	if (id == QUOTA_DEFAULTID) {
		id = 0;
		defaultq = 1;
	} else {
		defaultq = 0;
	}

	if (defaultq) {
		if (!prop_dictionary_set_cstring_nocopy(dict, "id",
		    "default")) {
			goto err;
		}
	} else {
		if (!prop_dictionary_set_uint32(dict, "id", id)) {
			goto err;
		}
	}

	return dict;

err:
	prop_object_release(dict);
	return NULL;
}

static int
vfs_quotactl_getall_addreply(prop_dictionary_t thisreply,
    const struct quotakey *key, const struct quotaval *val)
{
#define INITQVNAMES_ALL { \
    QUOTADICT_LIMIT_HARD, \
    QUOTADICT_LIMIT_SOFT, \
    QUOTADICT_LIMIT_USAGE, \
    QUOTADICT_LIMIT_ETIME, \
    QUOTADICT_LIMIT_GTIME \
    }
#define N_QV 5

	const char *val_names[] = INITQVNAMES_ALL;
	uint64_t vals[N_QV];
	prop_dictionary_t dict2;
	const char *objtypename;

	switch (key->qk_objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		objtypename = QUOTADICT_LTYPE_BLOCK;
		break;
	    case QUOTA_OBJTYPE_FILES:
		objtypename = QUOTADICT_LTYPE_FILE;
		break;
	    default:
		return EINVAL;
	}

	vals[0] = val->qv_hardlimit;
	vals[1] = val->qv_softlimit;
	vals[2] = val->qv_usage;
	vals[3] = val->qv_expiretime;
	vals[4] = val->qv_grace;
	dict2 = limits64toprop(vals, val_names, N_QV);
	if (dict2 == NULL)
		return ENOMEM;

	if (!prop_dictionary_set_and_rel(thisreply, objtypename, dict2))
		return ENOMEM;

	return 0;
}

static int
vfs_quotactl_getall(struct quotahandle *qh,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	struct quotacursor *cursor;
	struct quotakey *keys;
	struct quotaval *vals;
	unsigned loopmax = 8;
	unsigned loopnum;
	int num;
	int skipidtype;
	prop_array_t replies;
	int atend, atzero;
	struct quotakey *key;
	struct quotaval *val;
	id_t lastid;
	prop_dictionary_t thisreply;
	unsigned i;
	int error;

	assert(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);

	cursor = quota_opencursor(qh);
	if (cursor == NULL) {
		return errno;
	}

	keys = malloc(loopmax * sizeof(keys[0]));
	if (keys == NULL) {
		quotacursor_close(cursor);
		return ENOMEM;
	}
	vals = malloc(loopmax * sizeof(vals[0]));
	if (vals == NULL) {
		free(keys);
		quotacursor_close(cursor);
		return ENOMEM;
	}

	skipidtype = (q2type == QUOTA_IDTYPE_USER ?
		      QUOTA_IDTYPE_GROUP : QUOTA_IDTYPE_USER);
	quotacursor_skipidtype(cursor, skipidtype);
	/* ignore if it fails */

	replies = prop_array_create();
	if (replies == NULL) {
		error = ENOMEM;
		goto err;
	}

	thisreply = NULL;
	lastid = 0; /* value not actually referenced */
	atzero = 0;

	while (1) {
		atend = quotacursor_atend(cursor);
		if (atend) {
			break;
		}

		num = quotacursor_getn(cursor, keys, vals, loopmax);
		if (num < 0) {
			error = errno;
		} else {
			error = 0;
			loopnum = num;
		}

		if (error == EDEADLK) {
			/*
			 * transaction abort, start over
			 */

			if (quotacursor_rewind(cursor)) {
				error = errno;
				goto err;
			}

			quotacursor_skipidtype(cursor, skipidtype);
			/* ignore if it fails */

			prop_object_release(replies);
			replies = prop_array_create();
			if (replies == NULL) {
				error = ENOMEM;
				goto err;
			}

			thisreply = NULL;
			lastid = 0;
			atzero = 0;

			continue;
		}
		if (error) {
			goto err;
		}

		if (loopnum == 0) {
			/*
			 * This is not supposed to happen. However,
			 * allow a return of zero items once as long
			 * as something happens (including an atend
			 * indication) on the next pass. If it happens
			 * twice, warn and assume end of iteration.
			 */
			if (atzero) {
				warnx("zero items returned");
				break;
			}
			atzero = 1;
		} else {
			atzero = 0;
		}

		for (i = 0; i < loopnum; i++) {
			key = &keys[i];
			val = &vals[i];

			if (key->qk_idtype != q2type) {
				/* don't want this result */
				continue;
			}

			if (thisreply == NULL || key->qk_id != lastid) {
				lastid = key->qk_id;
				thisreply = vfs_quotactl_getall_makereply(key);
				if (thisreply == NULL) {
					error = ENOMEM;
					goto err;
				}
				/*
				 * Note: while we release our reference to
				 * thisreply here, we can (and do) continue to
				 * use the pointer in the loop because the
				 * copy attached to the replies array is not
				 * going away.
				 */
				if (!prop_array_add_and_rel(replies,
							    thisreply)) {
					error = ENOMEM;
					goto err;
				}
			}

			error = vfs_quotactl_getall_addreply(thisreply,
							     key, val);
			if (error) {
				goto err;
			}
		}
	}

	if (!prop_dictionary_set_and_rel(cmddict, "data", replies)) {
		replies = NULL;
		error = ENOMEM;
		goto err;
	}
	replies = NULL;
	error = 0;

 err:
	free(keys);
	free(vals);

	if (replies != NULL) {
		prop_object_release(replies);
	}

	quotacursor_close(cursor);

	if (error) {
		return error;
	}
	return 0;
}

static int
vfs_quotactl_clear(struct quotahandle *qh,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	prop_array_t replies;
	prop_object_iterator_t iter;
	prop_dictionary_t data;
	uint32_t id;
	int defaultq;
	const char *idstr;
	struct quotakey qk;
	int error;

	assert(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	assert(prop_object_type(datas) == PROP_TYPE_ARRAY);

	replies = prop_array_create();
	if (replies == NULL)
		return ENOMEM;

	iter = prop_array_iterator(datas);
	if (iter == NULL) {
		prop_object_release(replies);
		return ENOMEM;
	}

	while ((data = prop_object_iterator_next(iter)) != NULL) {
		if (!prop_dictionary_get_uint32(data, "id", &id)) {
			if (!prop_dictionary_get_cstring_nocopy(data, "id",
			    &idstr))
				continue;
			if (strcmp(idstr, "default"))
				continue;
			id = 0;
			defaultq = 1;
		} else {
			defaultq = 0;
		}

		qk.qk_idtype = q2type;
		qk.qk_id = defaultq ? QUOTA_DEFAULTID : id;
		qk.qk_objtype = QUOTA_OBJTYPE_BLOCKS;

		if (quota_delete(qh, &qk)) {
			error = errno;
			goto err;
		}

		qk.qk_idtype = q2type;
		qk.qk_id = defaultq ? QUOTA_DEFAULTID : id;
		qk.qk_objtype = QUOTA_OBJTYPE_FILES;

		if (quota_delete(qh, &qk)) {
			error = errno;
			goto err;
		}
	}

	prop_object_iterator_release(iter);
	if (!prop_dictionary_set_and_rel(cmddict, "data", replies)) {
		error = ENOMEM;
	} else {
		error = 0;
	}
	return error;
err:
	prop_object_iterator_release(iter);
	prop_object_release(replies);
	return error;
}

static int
vfs_quotactl_cmd(struct quotahandle *qh, prop_dictionary_t cmddict)
{
	int error;
	const char *cmd, *type;
	prop_array_t datas;
	int q2type;

	if (!prop_dictionary_get_cstring_nocopy(cmddict, "command", &cmd))
		return EINVAL;
	if (!prop_dictionary_get_cstring_nocopy(cmddict, "type", &type))
		return EINVAL;

	if (!strcmp(type, QUOTADICT_CLASS_USER)) {
		q2type = QUOTA_CLASS_USER;
	} else if (!strcmp(type, QUOTADICT_CLASS_GROUP)) {
		q2type = QUOTA_CLASS_GROUP;
	} else {
		/* XXX this is a bad errno for this case */
		return EOPNOTSUPP;
	}

	datas = prop_dictionary_get(cmddict, "data");
	if (datas == NULL || prop_object_type(datas) != PROP_TYPE_ARRAY)
		return EINVAL;

	prop_object_retain(datas);
	prop_dictionary_remove(cmddict, "data"); /* prepare for return */

	if (strcmp(cmd, "get version") == 0) {
		error = vfs_quotactl_getversion(qh, cmddict, q2type, datas);
	} else if (strcmp(cmd, "quotaon") == 0) {
		error = vfs_quotactl_quotaon(qh, cmddict, q2type, datas);
	} else if (strcmp(cmd, "quotaoff") == 0) {
		error = vfs_quotactl_quotaoff(qh, cmddict, q2type, datas);
	} else if (strcmp(cmd, "get") == 0) {
		error = vfs_quotactl_get(qh, cmddict, q2type, datas);
	} else if (strcmp(cmd, "set") == 0) {
		error = vfs_quotactl_put(qh, cmddict, q2type, datas);
	} else if (strcmp(cmd, "getall") == 0) {
		error = vfs_quotactl_getall(qh, cmddict, q2type, datas);
	} else if (strcmp(cmd, "clear") == 0) {
		error = vfs_quotactl_clear(qh, cmddict, q2type, datas);
	} else {
		/* XXX this a bad errno for this case */
		error = EOPNOTSUPP;
	}

	error = (prop_dictionary_set_int8(cmddict, "return",
	    error) ? 0 : ENOMEM);
	prop_object_release(datas);

	return error;
}

int
proplib_quotactl(struct quotahandle *qh, prop_dictionary_t dict)
{
	prop_dictionary_t cmddict;
	prop_array_t commands;
	prop_object_iterator_t iter;
	int error;

	error = quota_get_cmds(dict, &commands);
	if (error) {
		return error;
	}

	iter = prop_array_iterator(commands);
	if (iter == NULL) {
		return ENOMEM;
	}

	while ((cmddict = prop_object_iterator_next(iter)) != NULL) {
		if (prop_object_type(cmddict) != PROP_TYPE_DICTIONARY) {
			/* XXX shouldn't this be an error? */
			continue;
		}
		error = vfs_quotactl_cmd(qh, cmddict);
		if (error) {
			break;
		}
	}
	prop_object_iterator_release(iter);
	return error;
}
