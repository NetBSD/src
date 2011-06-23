/* $NetBSD: quotaprop.c,v 1.1.2.1 2011/06/23 14:17:49 cherry Exp $ */
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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/inttypes.h>
#include <sys/errno.h>

#include <quota/quotaprop.h>

/*
 * update values from value[] using dict entries whose key is stored
 * in name[]. Unknown keys are ignored. If update is false,
 * a key in name[] but not in dict is an error.
 * name[] may have NULL pointers to skip a value[]
 */
int
quotaprop_dict_get_uint64(prop_dictionary_t dict, uint64_t value[],
    const char *name[], int nvalues, bool update)
{
	int i;
	uint64_t v;

	for (i = 0; i < nvalues; i++) {
		if (name[i] == NULL)
			continue;
		if (!prop_dictionary_get_uint64(dict, name[i], &v)) {
			if (!update)
				return EINVAL;
		}
		value[i] = v;
	}
	return 0;
}

/*
 * convert a quota entry dictionary to in-memory array of uint64_t's
 */
int
proptoquota64(prop_dictionary_t data, uint64_t *values[], const char *valname[],
    int nvalues, const char *limname[], int nlimits)
{
	int i, error;
	prop_dictionary_t val;

	for (i = 0; i < nlimits; i++) {
		if (limname[i] == NULL)
			continue;
		if (!prop_dictionary_get_dict(data, limname[i], &val))
			return EINVAL;
		error = quotaprop_dict_get_uint64(val, values[i],
		    valname, nvalues, false);
		if (error)
			return error;
	}
	return 0;
}

int
quota_get_cmds(prop_dictionary_t qdict, prop_array_t *cmds)
{
	prop_number_t pn;
	prop_object_t o;

	pn = prop_dictionary_get(qdict, "interface version");
	if (pn == NULL)
		return EINVAL;
	if (prop_number_integer_value(pn) != 1)
		return EINVAL;

	o = prop_dictionary_get(qdict, "commands");
	if (o == NULL)
		return ENOMEM;
	 if(prop_object_type(o) != PROP_TYPE_ARRAY)
		return EINVAL;
	*cmds = o;
	return 0;
}


prop_dictionary_t
quota_prop_create(void)
{
	prop_dictionary_t dict = prop_dictionary_create();

	if (dict == NULL)
		return NULL;

	if (!prop_dictionary_set_uint8(dict, "interface version", 1)) {
		goto err;
	}
	return dict;
err:
	prop_object_release(dict);
	return NULL;
}

bool
quota_prop_add_command(prop_array_t arrcmd, const char *cmd, const char *type,
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

/* construct a dictionary using array of values and corresponding keys */
prop_dictionary_t
limits64toprop(uint64_t value[], const char *name[], int nvalues)
{
	int i;
	prop_dictionary_t dict1 = prop_dictionary_create();
	if (dict1 == NULL)
		return NULL;

	for (i = 0;  i < nvalues; i++) {
		if (name[i] == NULL)
			continue;
		if (!prop_dictionary_set_uint64(dict1, name[i], value[i])) {
			prop_object_release(dict1);
			return NULL;
		}
	}
	return dict1;
}

/*
 * construct a quota entry using provided array of values, array of values
 * names
 */
prop_dictionary_t
quota64toprop(uid_t uid, int def, uint64_t *values[], const char *valname[],
    int nvalues, const char *limname[], int nlimits)
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
		if (!prop_dictionary_set_uint32(dict1, "id", uid)) {
			goto err;
		}
	}
	for (i = 0; i < nlimits; i++) {
		if (limname[i] == NULL)
			continue;
		dict2 = limits64toprop(values[i], valname, nvalues);
		if (dict2 == NULL)
			goto err;
		if (!prop_dictionary_set_and_rel(dict1, limname[i],
		    dict2))
			goto err;
	}

	return dict1;
err:
	prop_object_release(dict1);
	return NULL;
}

const char *ufs_quota_entry_names[] = UFS_QUOTA_ENTRY_NAMES;
const char *ufs_quota_limit_names[] = QUOTA_LIMIT_NAMES;
const char *ufs_quota_class_names[] = QUOTA_CLASS_NAMES;
