/* 	$NetBSD: devfsd_rule.c,v 1.1.2.1 2007/12/08 22:05:05 mjf Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

/*
 * This code is a prototype for the rule matching code that will be part of
 * the devfsd daemon.
 *
 * TODO:
 *	- check that we're handling retaining/releasing objects correctly
 *	- possibly check for duplicate rules (if not too expensive)?
 *	- write code for devices list and associate rule with device
 */

#include <sys/queue.h>
#include <sys/types.h>

#include <prop/proplib.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "devfsd.h"

extern struct rlist_head  rule_list;

static int handle_match(const char *, const char *, struct devfs_rule *); 
static int handle_attributes(const char *, const char *, struct devfs_rule *);
static int handle_dict(prop_dictionary_t, const char *, struct devfs_rule *);
static prop_dictionary_t construct_rule(struct devfs_rule *);

int
rule_init(void)
{
	SLIST_INIT(&rule_list);
	return 0;
}

/*
 * Read the XML contained in 'filename' and internalize it. We then construct
 * a list of devfs rules from the rules written in the file.
 *
 * The structure of the config file is this:
 *
 * <dict>
 *	<key>rule-name</key>
 *	<dict>
 *		<key>match</key>
 *		<dict>
 *			<key>label</key>
 * 			<string>UUID-2234-0000</string>
 *		</dict>
 *
 *		<key>attribute</key>
 *		<dict>
 *			<key>mode</key>
 * 			<string>0644</string>
 *		</dict>
 *	</dict>
 * </dict>
 *
 */
int
rule_parsefile(const char *filename)
{
	int error;
	prop_dictionary_t dict, rule, section_dict;
	prop_object_iterator_t iter1, iter2;
	prop_object_t obj1, obj2;
	const char *section_string;
	struct devfs_rule *sr;

	dict = prop_dictionary_internalize_from_file(filename);

	if (dict == NULL) {
		errx(1, "%s: could not internalize dictionary\n", filename);
		return -1;
	}

	if ((iter1 = prop_dictionary_iterator(dict)) == NULL) {
		prop_object_release(dict);
		errx(1, "could not create iterator");
		return -1;
	}

	while ((obj1 = prop_object_iterator_next(iter1)) != NULL) {
		if ((sr = calloc(1, sizeof *sr)) == NULL) {
			err(1, "malloc");
			return -1;
		}

		sr->r_mode = sr->r_owner = sr->r_group = DEVFS_RULE_ATTR_UNSET;

		rule = prop_dictionary_get_keysym(dict, obj1);
		if (rule == NULL) {		
			fprintf(stderr, "%s: could not read rule\n", filename);
		    	return -1;
		}
		
		sr->r_name = prop_dictionary_keysym_cstring_nocopy(obj1);

		if ((iter2 = prop_dictionary_iterator(rule)) == NULL) {
			prop_object_release(rule);
			return -1;
		}

		while ((obj2 = prop_object_iterator_next(iter2)) != NULL) {
			section_dict = 
			    prop_dictionary_get_keysym(rule, obj2);

			section_string = 
			    prop_dictionary_keysym_cstring_nocopy(obj2);

			error = handle_dict(section_dict, section_string, sr);

			if (error)
				return error;
		}
		prop_object_iterator_release(iter2);

		SLIST_INSERT_HEAD(&rule_list, sr, r_next);
	}

	prop_object_iterator_release(iter1);
	return 0;
}

/*
 * This function parsers the keys of the dictionary which will either be
 * "match" or "attributes" and calls the relevant handler function. This
 * is step 3.
 */
static int
handle_dict(prop_dictionary_t d, const char *section, struct devfs_rule *sr)
{
	int error = 0;
	prop_object_iterator_t iter;
	prop_object_t obj;
	prop_string_t key;
	prop_object_t value;
	const char *key_string, *value_string;

	if ((iter = prop_dictionary_iterator(d)) == NULL)
		return -1;

	while ((obj = prop_object_iterator_next(iter)) != NULL) {
		key = prop_dictionary_get_keysym(d, obj);
		key_string = prop_dictionary_keysym_cstring_nocopy(obj);
		value = prop_dictionary_get(d, key_string);
		value_string = prop_string_cstring_nocopy(value);

		if (!strncmp(section, "match", strlen("match")))
			error = handle_match(key_string, value_string, sr);
		else if (!strncmp(section, "attributes", strlen("attributes")))
			error = handle_attributes(key_string, value_string, sr);
		else {
			errx(1, "invalid rule section\n");
			return -1;
		} 
	}

	if (error)
		return error;

	return 0;
}

/*
 * Handler function for match dictionaries. Step 4.
 */
static int
handle_match(const char *key, const char *value, struct devfs_rule *sr)
{
	if (!strncmp(key, "label", strlen("label"))) {
		sr->r_label = value;
	} else if (!strncmp(key, "manufacterer", strlen("manufacturer"))) {
		sr->r_manufacturer = value;
	} else if (!strncmp(key, "product", strlen("product"))) {
		sr->r_product = value;
	} else if (!strncmp(key, "drivername", strlen("drivername"))) {
		sr->r_drivername = value;
	} else {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

/*
 * Handler function for attribute dictionaries. Step 4.
 */
static int
handle_attributes(const char *key, const char *value, struct devfs_rule *sr)
{
	if (!strncmp(key, "filename", strlen("filename"))) {
		sr->r_filename = value;
	} else if (!strncmp(key, "mode", strlen("mode"))) {
		sr->r_mode = strtoul(value, (char **)NULL, 8);
	} else if (!strncmp(key, "uid", strlen("owner"))) {
		sr->r_owner = atoi(value);
	} else if (!strncmp(key, "gid", strlen("group"))) {
		sr->r_group = atoi(value);
	} else if (!strncmp(key, "visibility", strlen("visibility"))) {
		if (!strncmp(value, "visibile", strlen("visibile"))) {
			sr->r_flags &= ~DEVFS_INVISIBLE;
			sr->r_flags |= DEVFS_VISIBLE;
		} else if (!strncmp(value, "invisible", strlen("invisible"))) {
			sr->r_flags &= ~DEVFS_INVISIBLE;
			sr->r_flags |= DEVFS_INVISIBLE;
		} else {
			errno = EINVAL;
			return -1;
		}
	} else {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

/*
 * Use the global list of rules, 'rule_list', and construct the correct proplib
 * objects for them. Write these out to the file 'filename'.
 */
int
rule_writefile(const char *filename)
{
	struct devfs_rule *dr;
	prop_dictionary_t main_dict;
	prop_dictionary_t rule;

	main_dict = prop_dictionary_create();

	SLIST_FOREACH(dr, &rule_list, r_next) {
		if ((rule = construct_rule(dr)) == NULL) {
			/* XXX: avoid writing an incomplete config file */
			errx(1, "could not construct proplib obj from rule");
			goto error;
		}

		if (prop_dictionary_set(main_dict, dr->r_name, rule) != true) {
			errx(1, "could not add proplib obj to dictionary");
			goto error;
		}
	}

	if (prop_dictionary_externalize_to_file(main_dict, filename) != true) {
		errx(1, "could not externalize to %s", filename);
		goto error;
	}

	return 0;

error:
	prop_object_release(main_dict);
	return -1;
}

/*
 * Given a devfs rule we must construct dictionary to present it.
 */
static prop_dictionary_t
construct_rule(struct devfs_rule *dr)
{
	int error = 0;
	prop_dictionary_t rule, tmp1, tmp2;
	prop_string_t tmp_string;
	char buf[11];

	rule = prop_dictionary_create();
	tmp1 = prop_dictionary_create();
	tmp2 = prop_dictionary_create();

	if (rule == NULL || tmp1 == NULL || tmp2 == NULL) {
		errx(1, "could not create dictionary\n");
		return NULL;
	}

	/*
	 * First handle the match section.
	 *
	 * XXX: Currently we only allow one matching spec to be used, we
	 * may change this to allow a more fine-grained matching.
	 */
	if (dr->r_label != NULL) {
		tmp_string = prop_string_create_cstring_nocopy(dr->r_label);
		if (prop_dictionary_set(tmp1, "label", tmp_string) != true) {
			error = 1;
			goto out;
		}
	} else if (dr->r_manufacturer != NULL) {
		tmp_string = 
		    prop_string_create_cstring_nocopy(dr->r_manufacturer);
		if (prop_dictionary_set(tmp1, "manufacturer", 
		    tmp_string) != true) {
		    	error = 1;
			goto out;
		}
	} else if (dr->r_product != NULL) {
		tmp_string = prop_string_create_cstring_nocopy(dr->r_product);
		if (prop_dictionary_set(tmp1, "product", tmp_string) != true) {
			error = 1;
			goto out;
		}
	} else if (dr->r_drivername != NULL) {
		tmp_string = 
		    prop_string_create_cstring_nocopy(dr->r_drivername);
		if (prop_dictionary_set(tmp1, "drivername", tmp_string) != true){
			error = 1;
			goto out;
		}
	}

	if (prop_dictionary_set(rule, "match", tmp1) != true) {
		error = 1;
		goto out;
	}

	/*
	 * Now copy the attributes section
	 */
	if (dr->r_filename != NULL) {
		tmp_string = prop_string_create_cstring(dr->r_filename);
		if (prop_dictionary_set(tmp2, "filename", tmp_string) != true) {
			error = 1;
			goto out;
		}
	}

	if (dr->r_mode != DEVFS_RULE_ATTR_UNSET) {
		if (snprintf(buf, 5, "%o", dr->r_mode) > 4) {
			errx(1, "mode too large\n");
			error = 1;
			goto out;
		}
		tmp_string = prop_string_create_cstring(buf);

		if (prop_dictionary_set(tmp2, "mode", tmp_string) != true) {
			error = 1;
			goto out;
		}
	}

	if (dr->r_owner != DEVFS_RULE_ATTR_UNSET) {
		if (snprintf(buf, 11, "%u", dr->r_owner) > 10) {
			errx(1, "uid too large\n");
			error = 1;
			goto out;
		}

		printf("uid=%s\n", buf);
		tmp_string = prop_string_create_cstring(buf);

		if (prop_dictionary_set(tmp2, "uid", tmp_string) != true) {
			error = 1;
			goto out;
		}
	}

	if (dr->r_group != DEVFS_RULE_ATTR_UNSET) {
		if (snprintf(buf, 11, "%u", dr->r_group) > 10) {
			errx(1, "gid too large\n");
			error = 1;
			goto out;
		}
		tmp_string = prop_string_create_cstring(buf);

		if (prop_dictionary_set(tmp2, "gid", tmp_string) != true) {
			error = 1;
			goto out;
		}
	}

	if (prop_dictionary_set(rule, "attributes", tmp2) != true) {
		error = 1;
		goto out;
	}

out:
	return (error) ? NULL : rule;
}
