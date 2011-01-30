/* $NetBSD: quota2_prop.c,v 1.1.2.3 2011/01/30 00:25:19 bouyer Exp $ */
/*-
  * Copyright (c) 2010 Manuel Bouyer
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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/inttypes.h>
#include <sys/errno.h>

#include <ufs/ufs/quota2_prop.h>

const char *quota2_valnames[] = QUOTA2_VALNAMES_INIT;

prop_dictionary_t
prop_dictionary_get_dict(prop_dictionary_t dict, const char *key)
{
	prop_object_t o;
	o = prop_dictionary_get(dict, key);
	if (o == NULL || prop_object_type(o) != PROP_TYPE_DICTIONARY)
		return NULL;
	return o;

}

int
quota2_dict_get_q2v_limits(prop_dictionary_t dict, struct quota2_val *q2v,
    bool update)
{
	uint64_t vu;
	int64_t v;
	if (!prop_dictionary_get_uint64(dict, "soft", &vu)) {
		if (!update)
			return EINVAL;
	} else
		q2v->q2v_softlimit = vu;
	if (!prop_dictionary_get_uint64(dict, "hard", &vu)) {
		if (!update)
			return EINVAL;
	} else
		q2v->q2v_hardlimit = vu;
	if (!prop_dictionary_get_int64(dict, "grace time", &v)) {
		if (!update)
			return EINVAL;
	} else
		q2v->q2v_grace = v;
	return 0;
}

int
quota2_dict_update_q2e_limits(prop_dictionary_t data, struct quota2_entry *q2e)
{
	int i, error;
	prop_dictionary_t val;
	for (i = 0; i < NQ2V; i++) {
		val = prop_dictionary_get_dict(data, quota2_valnames[i]);
		if (val == NULL)
			continue;
		error = quota2_dict_get_q2v_limits(val, &q2e->q2e_val[i], 1);
		if (error)
			return error;
	}
	return 0;
}

int
quota2_dict_get_q2v_usage(prop_dictionary_t dict, struct quota2_val *q2v)
{
	uint64_t vu;
	int64_t v;
	int err;

	err = quota2_dict_get_q2v_limits(dict, q2v, false);
	if (err)
		return err;
	if (!prop_dictionary_get_uint64(dict, "usage", &vu))
		return EINVAL;
	q2v->q2v_cur = vu;
	if (!prop_dictionary_get_int64(dict, "expire time", &v))
		return EINVAL;
	q2v->q2v_time = v;
	return 0;
}

int
quota2_dict_get_q2e_usage(prop_dictionary_t data, struct quota2_entry *q2e)
{
	int i, error;
	prop_dictionary_t val;
	for (i = 0; i < NQ2V; i++) {
		val = prop_dictionary_get_dict(data, quota2_valnames[i]);
		if (val == NULL)
			return EINVAL;
		error = quota2_dict_get_q2v_usage(val, &q2e->q2e_val[i]);
		if (error)
			return error;
	}
	return 0;
}

int
quota2_get_cmds(prop_dictionary_t qdict, prop_array_t *cmds)
{
	prop_number_t pn;
	prop_object_t o;

	pn = prop_dictionary_get(qdict, "interface version");
	if (pn == NULL)
		return EINVAL;
	if (prop_number_integer_value(pn) != 1)
		return EINVAL;

	pn = prop_dictionary_get(qdict, "quota version");
	if (pn == NULL)
		return EINVAL;
	if (prop_number_integer_value(pn) != 2)
		return EINVAL;

	o = prop_dictionary_get(qdict, "commands");
	if (o == NULL)
		return ENOMEM;
	 if(prop_object_type(o) != PROP_TYPE_ARRAY)
		return EINVAL;
	*cmds = o;
	return 0;
}

bool
prop_array_add_and_rel(prop_array_t array, prop_object_t po)
{
	bool ret;
	if (po == NULL)
		return false;
	ret = prop_array_add(array, po);
	prop_object_release(po);
	return ret;
}

bool
prop_dictionary_set_and_rel(prop_dictionary_t dict, const char *key,
    prop_object_t po)
{
	bool ret;
	if (po == NULL)
		return false;
	ret = prop_dictionary_set(dict, key, po);
	prop_object_release(po);
	return ret;
}

prop_dictionary_t
quota2_prop_create(void)
{
	prop_dictionary_t dict = prop_dictionary_create();

	if (dict == NULL)
		return NULL;

	if (!prop_dictionary_set_uint8(dict, "interface version", 1)) {
		goto err;
	}
	if (!prop_dictionary_set_uint8(dict, "quota version", 2)) {
		goto err;
	}
	return dict;
err:
	prop_object_release(dict);
	return NULL;
}

bool
quota2_prop_add_command(prop_array_t arrcmd, const char *cmd, const char *type,
    prop_array_t data)
{
	prop_dictionary_t dict;

	dict = prop_dictionary_create();
	if (dict == NULL) {
		return false;
	}
	if (!prop_dictionary_set_cstring(dict, "type", type)) {
		goto err;
	}
	if (!prop_dictionary_set_cstring(dict, "command", cmd)) {
		goto err;
	}
	if (!prop_dictionary_set_and_rel(dict, "data", data)) {
		goto err;
	}
	if (!prop_array_add(arrcmd, dict)) {
		goto err;
	}
	prop_object_release(dict);
	return true;
err:
	prop_object_release(dict);
	return false;
}

prop_dictionary_t
q2vtoprop(struct quota2_val *q2v)
{
	prop_dictionary_t dict1 = prop_dictionary_create();

	if (dict1 == NULL)
		return NULL;
	if (!prop_dictionary_set_uint64(dict1, "hard", q2v->q2v_hardlimit)) {
		goto err;
	}
	if (!prop_dictionary_set_uint64(dict1, "soft", q2v->q2v_softlimit)) {
		goto err;
	}
	if (!prop_dictionary_set_uint64(dict1, "usage", q2v->q2v_cur)) {
		goto err;
	}
	if (!prop_dictionary_set_int64(dict1, "expire time", q2v->q2v_time)) {
		goto err;
	}
	if (!prop_dictionary_set_int64(dict1, "grace time", q2v->q2v_grace)) {
		goto err;
	}
	return dict1;
err:
	prop_object_release(dict1);
	return NULL;
	
}

prop_dictionary_t
q2etoprop(struct quota2_entry *q2e, int def)
{
	prop_dictionary_t dict1 = prop_dictionary_create();
	prop_dictionary_t dict2;
	int i;

	if (dict1 == NULL)
		return NULL;
	if (def) {
		if (!prop_dictionary_set_cstring_nocopy(dict1, "id",
		    "default")) {
			goto err;
		}
	} else {
		if (!prop_dictionary_set_uint32(dict1, "id", q2e->q2e_uid)) {
			goto err;
		}
	}
	for (i = 0; i < NQ2V; i++) {
		dict2 = q2vtoprop(&q2e->q2e_val[i]);
		if (dict2 == NULL)
			goto err;
		if (!prop_dictionary_set_and_rel(dict1, quota2_valnames[i],
		    dict2))
			goto err;
	}

	return dict1;
err:
	prop_object_release(dict1);
	return NULL;
}
