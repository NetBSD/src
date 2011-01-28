/*	$NetBSD: getvfsquota.c,v 1.1.2.1 2011/01/28 22:15:36 bouyer Exp $ */

/*-
  * Copyright (c) 2011 Manuel Bouyer
  * All rights reserved.
  * This software is distributed under the following condiions
  * compliant with the NetBSD foundation policy.
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
__RCSID("$NetBSD: getvfsquota.c,v 1.1.2.1 2011/01/28 22:15:36 bouyer Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <string.h>

#include <sys/types.h>

#include <ufs/ufs/quota2_prop.h>
#include <sys/quota.h>

#include <getvfsquota.h>

const char *qfextension[] = INITQFNAMES;

/* retrieve quotas from vfs, for the given user id */
int
getvfsquota(const char *mp, struct quota2_entry *q2e, long id, int type,
    int defaultq, int debug)
{
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int error;
	int8_t error8;
	bool ret;

	dict = quota2_prop_create();
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
		
	if (!prop_array_add(datas, data))
		err(1, "prop_array_add(data)");
	prop_object_release(data);
	if (!quota2_prop_add_command(cmds, "get", qfextension[type], datas))
		err(1, "prop_add_command");
	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");
	if (debug)
		printf("message to kernel:\n%s\n",
		    prop_dictionary_externalize(dict));

	if (!prop_dictionary_send_syscall(dict, &pref))
		err(1, "prop_dictionary_send_syscall");
	prop_object_release(dict);

	if (quotactl(mp, &pref) != 0)
		err(1, "quotactl");
	
	if ((error = prop_dictionary_recv_syscall(&pref, &dict)) != 0) {
		errx(1, "prop_dictionary_recv_syscall: %s\n",
		    strerror(error));
	}
	if (debug)
		printf("reply from kernel:\n%s\n",
		    prop_dictionary_externalize(dict));
	if ((error = quota2_get_cmds(dict, &cmds)) != 0) {
		errx(1, "quota2_get_cmds: %s\n",
		    strerror(error));
	}
	/* only one command, no need to iter */
	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL)
		err(1, "prop_array_get(cmd)");

	if (!prop_dictionary_get_int8(cmd, "return", &error8))
		err(1, "prop_get(return)");

	if (error8) {
		if (error8 != ENOENT && error8 != ENODEV) {
			if (defaultq)
				fprintf(stderr, "get default %s quota: %s\n",
				    qfextension[type], strerror(error8));
			else 
				fprintf(stderr, "get %s quota for %ld: %s\n",
				    qfextension[type], id, strerror(error8));
		}
		prop_object_release(dict);
		return (0);
	}
	datas = prop_dictionary_get(cmd, "data");
	if (datas == NULL)
		err(1, "prop_dict_get(datas)");

	/* only one data, no need to iter */
	if (prop_array_count(datas) == 0) {
		/* no quota for this user/group */
		prop_object_release(dict);
		return (0);
	}
	
	data = prop_array_get(datas, 0);
	if (data == NULL)
		err(1, "prop_array_get(data)");

	error = quota2_dict_get_q2e_usage(data, q2e);
	if (error) {
		errx(1, "quota2_dict_get_q2e_usage: %s\n",
		    strerror(error));
	}
	prop_object_release(dict);
	return (1);
}
