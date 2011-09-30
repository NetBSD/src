/*	$NetBSD: getvfsquota.c,v 1.8 2011/09/30 22:08:19 jym Exp $ */

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
__RCSID("$NetBSD: getvfsquota.c,v 1.8 2011/09/30 22:08:19 jym Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <string.h>

#include <sys/types.h>

#include <quota/quotaprop.h>
#include <sys/quota.h>

#include "getvfsquota.h"

/* private version of getufsquota() */
int
getvfsquota(const char *mp, struct ufs_quota_entry *qv, int8_t *versp,
    uint32_t id, int type, int defaultq, int debug)
{
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	prop_object_iterator_t iter;
	struct plistref pref;
	int8_t error8;
	bool ret;
	int retval = 0;

	dict = quota_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();
	data = prop_dictionary_create();

	if (dict == NULL || cmds == NULL || datas == NULL || data == NULL)
		errx(1, "can't allocate proplist");

	if (defaultq)
		ret = prop_dictionary_set_cstring(data, "id", "default");
	else
		ret = prop_dictionary_set_uint32(data, "id", id);
	if (!ret)
		err(1, "prop_dictionary_set(id)");
		
	if (!prop_array_add_and_rel(datas, data))
		err(1, "prop_array_add(data)");
	if (!quota_prop_add_command(cmds, "get",
	    ufs_quota_class_names[type], datas))
		err(1, "prop_add_command");
	if (!quota_prop_add_command(cmds, "get version",
	    ufs_quota_class_names[type],
	    prop_array_create()))
		err(1, "prop_add_command");
	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");
	if (debug)
		printf("message to kernel:\n%s\n",
		    prop_dictionary_externalize(dict));

	if (prop_dictionary_send_syscall(dict, &pref) != 0)
		err(1, "prop_dictionary_send_syscall");
	prop_object_release(dict);

	if (quotactl(mp, &pref) != 0)
		err(1, "quotactl");
	
	if (prop_dictionary_recv_syscall(&pref, &dict) != 0)
		err(1, "prop_dictionary_recv_syscall");
	if (debug)
		printf("reply from kernel:\n%s\n",
		    prop_dictionary_externalize(dict));
	if ((errno = quota_get_cmds(dict, &cmds)) != 0)
		err(1, "quota_get_cmds");

	iter = prop_array_iterator(cmds);
	if (iter == NULL)
		err(1, "prop_array_iterator(cmds)");

	while ((cmd = prop_object_iterator_next(iter)) != NULL) {
		const char *cmdstr;
		if (!prop_dictionary_get_cstring_nocopy(cmd, "command",
		    &cmdstr))
			err(1, "prop_get(command)");
		if (!prop_dictionary_get_int8(cmd, "return", &error8))
			err(1, "prop_get(return)");

		if (error8) {
			if (error8 != ENOENT && error8 != ENODEV) {
				errno = error8;
				if (defaultq) {
					warn("get default %s quota",
					    ufs_quota_class_names[type]);
				} else {
					warn("get %s quota for %u",
					    ufs_quota_class_names[type], id);
				}
			}
			prop_object_release(dict);
			return 0;
		}
		datas = prop_dictionary_get(cmd, "data");
		if (datas == NULL)
			err(1, "prop_dict_get(datas)");

		if (strcmp("get version", cmdstr) == 0) {
			data = prop_array_get(datas, 0);
			if (data == NULL)
				err(1, "prop_array_get(version)");
			if (!prop_dictionary_get_int8(data, "version", versp))
				err(1, "prop_get_int8(version)");
			continue;
		}
		if (strcmp("get", cmdstr) != 0)
			err(1, "unknown command %s in reply", cmdstr);
				
		/* only one data, no need to iter */
		if (prop_array_count(datas) > 0) {
			uint64_t *values[QUOTA_NLIMITS];
			data = prop_array_get(datas, 0);
			if (data == NULL)
				err(1, "prop_array_get(data)");

			values[QUOTA_LIMIT_BLOCK] =
			    &qv[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit;
			values[QUOTA_LIMIT_FILE] =
			    &qv[QUOTA_LIMIT_FILE].ufsqe_hardlimit;

			errno = proptoquota64(data, values,
			     ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
			     ufs_quota_limit_names, QUOTA_NLIMITS);
			if (errno)
				err(1, "proptoquota64");
			retval = 1;
		}
	}
	prop_object_release(dict);
	return retval;
}
