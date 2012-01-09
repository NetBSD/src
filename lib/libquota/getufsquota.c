/*	$NetBSD: getufsquota.c,v 1.5 2012/01/09 15:28:31 dholland Exp $ */

/*-
  * Copyright (c) 2011 Manuel Bouyer
  * All rights reserved.
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
__RCSID("$NetBSD: getufsquota.c,v 1.5 2012/01/09 15:28:31 dholland Exp $");

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include <quota/quotaprop.h>
#include <quota/quota.h>

#include <quota.h>

/*
 * Return true if QV contains any actual information.
 *
 * XXX when qv_grace is not available it should be set to QUOTA_NOTIME,
 * not zero, but this is currently not always the case.
 */
static int
quotaval_nonempty(const struct quotaval *qv)
{
	if (qv->qv_hardlimit != QUOTA_NOLIMIT ||
	    qv->qv_softlimit != QUOTA_NOLIMIT ||
	    qv->qv_usage != 0 ||
	    qv->qv_expiretime != QUOTA_NOTIME ||
	    (qv->qv_grace != QUOTA_NOTIME && qv->qv_grace != 0)) {
		return 1;
	}
	return 0;
}

/*
 * "retrieve quotas with ufs semantics from vfs, for the given user id"
 *
 * What this actually does is: for mount point MP, and id ID, which
 * can be either a uid or a gid depending on what string CLASS
 * contains, fetch both block and file quotas and store them in QV,
 * namely in qv[0] and qv[1] respectively.
 */
int
getufsquota(const char *mp, struct quotaval *qv, uid_t id,
    const char *class)
{
#if 1
	struct quotakey qk;
	struct quotahandle *qh;

	qh = quota_open(mp);
	if (qh == NULL) {
		return -1;
	}

	qk.qk_id = id;
	if (!strcmp(class, QUOTADICT_CLASS_USER)) {
		qk.qk_idtype = QUOTA_IDTYPE_USER;
	} else if (!strcmp(class, QUOTADICT_CLASS_GROUP)) {
		qk.qk_idtype = QUOTA_IDTYPE_GROUP;
	} else {
		errno = EINVAL;
		return -1;
	}

	/*
	 * Use explicit indexes on qv[] to make sure this continues
	 * to work the way it used to (for compat) independent of
	 * what might happen to the symbolic constants.
	 */

	qk.qk_objtype = QUOTA_OBJTYPE_BLOCKS;
	if (quota_get(qh, &qk, &qv[0]) < 0) {
		return -1;
	}

	qk.qk_objtype = QUOTA_OBJTYPE_FILES;
	if (quota_get(qh, &qk, &qv[1]) < 0) {
		return -1;
	}

	quota_close(qh);

	if (quotaval_nonempty(&qv[0]) || quotaval_nonempty(&qv[1])) {
		return 1;
	}

	return 0;

#else /* old code for reference */
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int8_t error8;
	int retval;
	const char *cmdstr;

	dict = quota_prop_create();
	if (dict == NULL) {
		errno = ENOMEM;
		goto end_dict;
	}
	cmds = prop_array_create();
	if (cmds == NULL) {
		errno = ENOMEM;
		goto end_cmds;
	}
	datas = prop_array_create();
	if (datas == NULL) {
		errno = ENOMEM;
		goto end_datas;
	}
	data = prop_dictionary_create();
	if (data == NULL) {
		errno = ENOMEM;
		goto end_data;
	}

	if (!prop_dictionary_set_uint32(data, "id", id)) {
		errno = ENOMEM;
		goto end_data;
	}
	if (!prop_array_add_and_rel(datas, data)) {
		errno = ENOMEM;
		goto end_datas;
	}
	if (!quota_prop_add_command(cmds, "get", class,  datas)) {
		errno = ENOMEM;
		goto end_cmds;
	}
	if (!prop_dictionary_set(dict, "commands", cmds)) {
		errno = ENOMEM;
		goto end_dict;
	}

	if (prop_dictionary_send_syscall(dict, &pref) != 0)
		goto end_dict;
	prop_object_release(dict);

	if (quotactl(mp, &pref) != 0)
		return -1;
	
	if (prop_dictionary_recv_syscall(&pref, &dict) != 0)
		return -1;
	
	if ((errno = quota_get_cmds(dict, &cmds)) != 0)
		goto end_dict;

	/* only one command, no need to iter */

	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL) {
		errno = EINVAL;
		goto end_dict;
	}

	if (!prop_dictionary_get_cstring_nocopy(cmd, "command",
	    &cmdstr)) {
		errno = EINVAL;
		goto end_dict;
	}

	if (strcmp("get", cmdstr) != 0) {
		errno = EINVAL;
		goto end_dict;
	}
			
	if (!prop_dictionary_get_int8(cmd, "return", &error8)) {
		errno = EINVAL;
		goto end_dict;
	}

	if (error8) {
		prop_object_release(dict);
		if (error8 != ENOENT && error8 != ENODEV) {
			errno = error8;
			return -1;
		}
		return 0;
	}
	datas = prop_dictionary_get(cmd, "data");
	if (datas == NULL) {
		errno = EINVAL;
		goto end_dict;
	}

	/* only one data, no need to iter */
	if (prop_array_count(datas) > 0) {
		uint64_t *values[QUOTA_NLIMITS];

		data = prop_array_get(datas, 0);
		if (data == NULL) {
			errno = EINVAL;
			goto end_dict;
		}
		values[QUOTA_LIMIT_BLOCK] =
		    &qv[QUOTA_LIMIT_BLOCK].qv_hardlimit;
		values[QUOTA_LIMIT_FILE] =
		    &qv[QUOTA_LIMIT_FILE].qv_hardlimit;

		errno = proptoquota64(data, values, ufs_quota_entry_names,
		    UFS_QUOTA_NENTRIES, ufs_quota_limit_names, QUOTA_NLIMITS);
		if (errno)
			goto end_dict;
		retval = 1;
	} else {
		retval = 0;
	}
	prop_object_release(dict);
	return retval;

end_data:
	prop_object_release(data);
end_datas:
	prop_object_release(datas);
end_cmds:
	prop_object_release(cmds);
end_dict:
	prop_object_release(dict);
	return -1;
#endif
}
