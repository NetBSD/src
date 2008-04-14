/* 	$NetBSD: devfsd_rule.c,v 1.1.8.4 2008/04/14 16:23:57 mjf Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

struct attrs_handled {
	int file_handled;
	int m_handled;
	int u_handled;
	int g_handled;
	int flags_handled;
}; 

static int handle_match(const char *, char *, struct devfs_rule *); 
static int handle_attributes(const char *, char *, struct devfs_rule *);
static int handle_dict(prop_dictionary_t, const char *, struct devfs_rule *);
static prop_dictionary_t construct_rule(struct devfs_rule *);
/*static void rule_store_attributes(struct devfs_rule *rule, 
    struct devfsctl_specnode_attr *di, struct attrs_handled *ah, int store);*/

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
		syslog(LOG_ERR, "%s: could not internalize dictionary\n", 
		    filename);
		return -1;
	}

	if ((iter1 = prop_dictionary_iterator(dict)) == NULL) {
		prop_object_release(dict);
		syslog(LOG_ERR, "could not create iterator");
		return -1;
	}

	while ((obj1 = prop_object_iterator_next(iter1)) != NULL) {
		if ((sr = calloc(1, sizeof *sr)) == NULL) {
			syslog(LOG_ERR, "calloc failed");
			return -1;
		}

		sr->r_mode = sr->r_uid = sr->r_gid = DEVFS_RULE_ATTR_UNSET;
		sr->r_flags = -1;	/* default visibility unspecified */

		TAILQ_INIT(&sr->r_pairing);

		rule = prop_dictionary_get_keysym(dict, obj1);
		if (rule == NULL) {		
			syslog(LOG_ERR, "%s: could not read rule\n", filename);
		    	return -1;
		}
		
		sr->r_name = prop_dictionary_keysym_cstring_nocopy(obj1);

		if ((iter2 = prop_dictionary_iterator(rule)) == NULL) {
			prop_object_release(rule);
			syslog(LOG_ERR, "could not read rule");
			return -1;
		}

		while ((obj2 = prop_object_iterator_next(iter2)) != NULL) {
			section_dict = 
			    prop_dictionary_get_keysym(rule, obj2);

			section_string = 
			    prop_dictionary_keysym_cstring_nocopy(obj2);

			error = handle_dict(section_dict, section_string, sr);

			if (error) {
				syslog(LOG_ERR, "rule: match/attribute error");
				return error;
			}
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
	const char *key_string;
	char *value_string;

	if ((iter = prop_dictionary_iterator(d)) == NULL) {
		error = -1;
		goto out;
	}

	while ((obj = prop_object_iterator_next(iter)) != NULL) {
		key = prop_dictionary_get_keysym(d, obj);
		key_string = prop_dictionary_keysym_cstring_nocopy(obj);
		value = prop_dictionary_get(d, key_string);
		value_string = prop_string_cstring(value);

		if (!strncmp(section, "match", strlen("match")))
			error = handle_match(key_string, value_string, sr);
		else if (!strncmp(section, "attributes", strlen("attributes")))
			error = handle_attributes(key_string, value_string, sr);
		else {
			syslog(LOG_ERR, "invalid rule section\n");
			error = EINVAL;
		} 

		if (error)
			goto out;
	}

out:
	return error;
}

/*
 * Handler function for match dictionaries. Step 4.
 */
static int
handle_match(const char *key, char *value, struct devfs_rule *sr)
{
	int error = 0;

	if (!strncmp(key, "label", strlen("label")))
		sr->r_label = value;
	else if (!strncmp(key, "manufacterer", strlen("manufacturer")))
		sr->r_manufacturer = value;
	else if (!strncmp(key, "product", strlen("product")))
		sr->r_product = value;
	else if (!strncmp(key, "drivername", strlen("drivername")))
		sr->r_drivername = value;
	else if (!strncmp(key, "fstype", strlen("fstype")))
		sr->r_disk.r_fstype = value;
	else if (!strncmp(key, "wedge_name", strlen("wedge_name")))
		sr->r_disk.r_wident = value;
	else {
		syslog(LOG_ERR, "invalid match rule");
		error = EINVAL;
	}

	return error;
}

/*
 * Handler function for attribute dictionaries. Step 4.
 */
static int
handle_attributes(const char *key, char *value, struct devfs_rule *sr)
{
	int error = 0;

	if (!strncmp(key, "filename", strlen("filename")))
		sr->r_filename = value;

	if (!strncmp(key, "mode", strlen("mode")))
		sr->r_mode = strtoul(value, (char **)NULL, 8);

	if (!strncmp(key, "uid", strlen("uid")))
		sr->r_uid = atoi(value);

	if (!strncmp(key, "gid", strlen("gid")))
		sr->r_gid = atoi(value);

	if (!strncmp(key, "visibility", strlen("visibility"))) {
		if (!strncmp(value, "visible", strlen("visible")))
			sr->r_flags = DEVFS_VISIBLE;
		else if (!strncmp(value, "invisible", strlen("invisible")))
			sr->r_flags = DEVFS_INVISIBLE;
		else
			error = EINVAL;
	}

	return error;
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
		if (prop_dictionary_set(tmp1, "drivername", 
		    tmp_string) != true) {
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

	if (dr->r_uid != DEVFS_RULE_ATTR_UNSET) {
		if (snprintf(buf, 11, "%u", dr->r_uid) > 10) {
			errx(1, "uid too large\n");
			error = 1;
			goto out;
		}

		tmp_string = prop_string_create_cstring(buf);

		if (prop_dictionary_set(tmp2, "uid", tmp_string) != true) {
			error = 1;
			goto out;
		}
	}

	if (dr->r_gid != DEVFS_RULE_ATTR_UNSET) {
		if (snprintf(buf, 11, "%u", dr->r_gid) > 10) {
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

/*
 * Rebuild the attributes section of a rule. If a rule already specified
 * one of the attributes and that attribute has changed then replace the
 * value of the attribute in that rule. If no rule currently exists to
 * handle the leftover attributes of 'di' then we create a new rule that
 * contains their values.
 */
int
rule_rebuild_attr(struct devfs_dev *dev)
{
	/*
	struct devfs_rule *rule, *new_rule;
	struct devfsctl_specnode_attr *di = &dev->d_dev;
	struct attrs_handled ah;
	struct devfs_node *node;
	struct rule2dev *rd;
	char *mntpath;
	struct devfs_mount *dmp;

	ah.file_handled = (di->d_filename == NULL) ? 0 : 1;
	ah.m_handled = ATTR_ISSET(di->d_mode);
	ah.u_handled = ATTR_ISSET(di->d_uid);
	ah.g_handled = ATTR_ISSET(di->d_gid);
	ah.flags_handled = ATTR_ISSET(di->d_flags);
	*/

	/* existing rules */

	/* 
	 * TODO: Figure out how to update rules and deal with multiple
	 * mount points. For example, if I chmod a special node in /priv_dev
	 * do I want a rule creating for that mount only? Or for all mounts?
	TAILQ_FOREACH(rd, &dev->d_pairing, r_next_rule) {
		mntpath = rd->r_rule->r_mntpath;
		
		SLIST_FOREACH(node, &dev->d_node_head, n_next) {
	 */
			/*
			 * Determine if this rule has been applied to
			 * this node. The only reason that this rule _hasnt_
			 * been applied to this node is if the rule is
			 * associated with a specific mount point.
			 */
			/*
			if (mntpath[0] != '\0') {
				dmp = mount_lookup(node.n_cookie.sc_mount);
				if (strcmp(mntpath, dmp->m_pathname) != 0)
					continue;
			}

			rule_store_attributes(rule, &node.n_attr, &ah, 0);
		}
	}
	*/

	/* Now find all attributes that weren't taken care of */
	/*
	if (ah.file_handled != 0 || ah.m_handled != 0 || ah.u_handled != 0
	    || ah.g_handled != 0 || ah.flags_handled != 0) {

		new_rule = malloc(sizeof(*new_rule));
		if (new_rule == NULL) {
			warn("could not fully rebuild rule attributes\n");
			return -1;
		}
		rule_store_attributes(new_rule, &node.n_attr, &ah, 1);
	}
	*/

	return 0;
}

/*
static void
rule_store_attributes(struct devfs_rule *rule, 
    struct devfsctl_specnode_attr *di, struct attrs_handled *ah, int store)
    
{
	int error = 0;

	if (ah->file_handled != 0) {
		size_t len = strlen(di->d_filename);

		if (rule->r_filename != NULL || store) {
			if (rule->r_filename != NULL)
				free(rule->r_filename);

			rule->r_filename = malloc(len + 1);

			if (rule->r_filename == NULL) {
				error = 1;
				syslog(LOG_WARNING, "could not alloc memory");
			} else {
				strncpy(rule->r_filename, di->d_filename, len);
				rule->r_filename[len] = '\0';
			}

			if (!error)
				ah->file_handled = 0;
		}
	}

	if (ah->m_handled != 0) {
		if (ATTR_ISSET(rule->r_mode) || store) {
			rule->r_mode = di->d_mode;
			ah->m_handled = 0;
		}
	}

	if (ah->u_handled != 0) {
		if (ATTR_ISSET(rule->r_uid) || store) {
			rule->r_uid = di->d_uid;
			ah->u_handled = 0;
		}
	}

	if (ah->g_handled != 0) {
		if (ATTR_ISSET(rule->r_gid) || store) {
			rule->r_gid = di->d_gid;
			ah->g_handled = 0;
		}
	}

	if (ah->flags_handled != 0) {
		if (ATTR_ISSET(rule->r_flags) || store) {
			rule->r_flags = di->d_flags;
			ah->flags_handled = 0;
		}
	}
}
*/
