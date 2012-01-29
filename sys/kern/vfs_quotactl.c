/*	$NetBSD: vfs_quotactl.c,v 1.19 2012/01/29 06:54:34 dholland Exp $	*/

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
 * Note that both of the copyrights above are moderately spurious;
 * this code should almost certainly have the Copyright 2010 Manuel
 * Bouyer notice and license found in e.g. sys/ufs/ufs/quota2_subr.c.
 * However, they're what was on the files this code was sliced out of.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_quotactl.c,v 1.19 2012/01/29 06:54:34 dholland Exp $");

#include <sys/mount.h>
#include <sys/quota.h>
#include <sys/quotactl.h>
#include <quota/quotaprop.h>

static int
vfs_quotactl_getversion(struct mount *mp,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	prop_array_t replies;
	prop_dictionary_t data;
	int q2version;
	struct vfs_quotactl_args args;
	int error;

	KASSERT(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	KASSERT(prop_object_type(datas) == PROP_TYPE_ARRAY);

	args.qc_type = QCT_GETVERSION;
	args.u.getversion.qc_version_ret = &q2version;
	error = VFS_QUOTACTL(mp, QUOTACTL_GETVERSION, &args);
	if (error) {
		return error;
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

	return error;
}

static int
vfs_quotactl_quotaon(struct mount *mp,
		     prop_dictionary_t cmddict, int q2type,
		     prop_array_t datas)
{
	struct vfs_quotactl_args args;

	args.qc_type = QCT_PROPLIB;
	args.u.proplib.qc_cmddict = cmddict;
	args.u.proplib.qc_q2type = q2type;
	args.u.proplib.qc_datas = datas;
	return VFS_QUOTACTL(mp, QUOTACTL_QUOTAON, &args);
}

static int
vfs_quotactl_quotaoff(struct mount *mp,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	struct vfs_quotactl_args args;

	args.qc_type = QCT_PROPLIB;
	args.u.proplib.qc_cmddict = cmddict;
	args.u.proplib.qc_q2type = q2type;
	args.u.proplib.qc_datas = datas;
	return VFS_QUOTACTL(mp, QUOTACTL_QUOTAOFF, &args);
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
vfs_quotactl_get(struct mount *mp,
			prop_dictionary_t cmddict, int idtype,
			prop_array_t datas)
{
	prop_object_iterator_t iter;
	prop_dictionary_t data;
	prop_array_t replies;
	uint32_t id;
	const char *idstr;
	struct vfs_quotactl_args args;
	struct quotakey qk;
	struct quotaval blocks, files;
	int error;

	KASSERT(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	KASSERT(prop_object_type(datas) == PROP_TYPE_ARRAY);

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

		args.qc_type = QCT_GET;
		args.u.get.qc_key = &qk;
		args.u.get.qc_ret = &blocks;
		error = VFS_QUOTACTL(mp, QUOTACTL_GET, &args);
		if (error == EPERM) {
			/* XXX does this make sense? */
			continue;
		} else if (error == ENOENT) {
			/* XXX does *this* make sense? */
			continue;
		} else if (error) {
			goto fail;
		}

		qk.qk_objtype = QUOTA_OBJTYPE_FILES;

		args.qc_type = QCT_GET;
		args.u.get.qc_key = &qk;
		args.u.get.qc_ret = &files;
		error = VFS_QUOTACTL(mp, QUOTACTL_GET, &args);
		if (error == EPERM) {
			/* XXX does this make sense? */
			continue;
		} else if (error == ENOENT) {
			/* XXX does *this* make sense? */
			continue;
		} else if (error) {
			goto fail;
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
vfs_quotactl_put(struct mount *mp,
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
	struct vfs_quotactl_args args;
	int error;

	KASSERT(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	KASSERT(prop_object_type(datas) == PROP_TYPE_ARRAY);

	replies = prop_array_create();
	if (replies == NULL)
		return ENOMEM;

	iter = prop_array_iterator(datas);
	if (iter == NULL) {
		prop_object_release(replies);
		return ENOMEM;
	}

	while ((data = prop_object_iterator_next(iter)) != NULL) {

		KASSERT(prop_object_type(data) == PROP_TYPE_DICTIONARY);

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

		args.qc_type = QCT_PUT;
		args.u.put.qc_key = &qk;
		args.u.put.qc_val = &blocks;
		error = VFS_QUOTACTL(mp, QUOTACTL_PUT, &args);
		if (error) {
			goto err;
		}

		qk.qk_idtype = q2type;
		qk.qk_id = defaultq ? QUOTA_DEFAULTID : id;
		qk.qk_objtype = QUOTA_OBJTYPE_FILES;

		args.qc_type = QCT_PUT;
		args.u.put.qc_key = &qk;
		args.u.put.qc_val = &files;
		error = VFS_QUOTACTL(mp, QUOTACTL_PUT, &args);
		if (error) {
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
vfs_quotactl_getall(struct mount *mp,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	struct vfs_quotactl_args args;

	args.qc_type = QCT_PROPLIB;
	args.u.proplib.qc_cmddict = cmddict;
	args.u.proplib.qc_q2type = q2type;
	args.u.proplib.qc_datas = datas;
	return VFS_QUOTACTL(mp, QUOTACTL_GETALL, &args);
}

static int
vfs_quotactl_clear(struct mount *mp,
			prop_dictionary_t cmddict, int q2type,
			prop_array_t datas)
{
	prop_array_t replies;
	prop_object_iterator_t iter;
	prop_dictionary_t data;
	uint32_t id;
	int defaultq;
	const char *idstr;
	struct vfs_quotactl_args args;
	int error;

	KASSERT(prop_object_type(cmddict) == PROP_TYPE_DICTIONARY);
	KASSERT(prop_object_type(datas) == PROP_TYPE_ARRAY);

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

		args.qc_type = QCT_DELETE;
		args.u.delete.qc_idtype = q2type;
		args.u.delete.qc_id = id;
		args.u.delete.qc_defaultq = defaultq;
		args.u.delete.qc_objtype = QUOTA_OBJTYPE_BLOCKS;
		error = VFS_QUOTACTL(mp, QUOTACTL_DELETE, &args);
		if (error) {
			goto err;
		}

		args.qc_type = QCT_DELETE;
		args.u.delete.qc_idtype = q2type;
		args.u.delete.qc_id = id;
		args.u.delete.qc_defaultq = defaultq;
		args.u.delete.qc_objtype = QUOTA_OBJTYPE_FILES;
		error = VFS_QUOTACTL(mp, QUOTACTL_DELETE, &args);
		if (error) {
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
vfs_quotactl_cmd(struct mount *mp, prop_dictionary_t cmddict)
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
		error = vfs_quotactl_getversion(mp, cmddict, q2type, datas);
	} else if (strcmp(cmd, "quotaon") == 0) {
		error = vfs_quotactl_quotaon(mp, cmddict, q2type, datas);
	} else if (strcmp(cmd, "quotaoff") == 0) {
		error = vfs_quotactl_quotaoff(mp, cmddict, q2type, datas);
	} else if (strcmp(cmd, "get") == 0) {
		error = vfs_quotactl_get(mp, cmddict, q2type, datas);
	} else if (strcmp(cmd, "set") == 0) {
		error = vfs_quotactl_put(mp, cmddict, q2type, datas);
	} else if (strcmp(cmd, "getall") == 0) {
		error = vfs_quotactl_getall(mp, cmddict, q2type, datas);
	} else if (strcmp(cmd, "clear") == 0) {
		error = vfs_quotactl_clear(mp, cmddict, q2type, datas);
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
vfs_quotactl(struct mount *mp, prop_dictionary_t dict)
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
		error = vfs_quotactl_cmd(mp, cmddict);
		if (error) {
			break;
		}
	}
	prop_object_iterator_release(iter);
	return error;
}
