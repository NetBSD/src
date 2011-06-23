/*	$NetBSD: getufsquota.c,v 1.1.2.1 2011/06/23 14:18:40 cherry Exp $ */

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
__RCSID("$NetBSD: getufsquota.c,v 1.1.2.1 2011/06/23 14:18:40 cherry Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <string.h>

#include <sys/types.h>

#include <quota/quotaprop.h>
#include <quota/quota.h>

/* retrieve quotas with ufs semantics from vfs, for the given user id */
int
getufsquota(const char *mp, struct ufs_quota_entry *qv, uid_t id,
    const char *class)
{
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

	if (!prop_dictionary_send_syscall(dict, &pref))
		goto end_dict;
	prop_object_release(dict);

	if (quotactl(mp, &pref) != 0)
		return -1;
	
	if ((errno = prop_dictionary_recv_syscall(&pref, &dict)) != 0)
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
		    &qv[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit;
		values[QUOTA_LIMIT_FILE] =
		    &qv[QUOTA_LIMIT_FILE].ufsqe_hardlimit;

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
}
