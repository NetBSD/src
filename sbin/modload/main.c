/*	$NetBSD: main.c,v 1.14.12.1 2013/02/25 00:28:08 tls Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.14.12.1 2013/02/25 00:28:08 tls Exp $");
#endif /* !lint */

#include <sys/module.h>
#include <sys/queue.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include <prop/proplib.h>

#include "prog_ops.h"

int		main(int, char **);
static void	parse_bool_param(prop_dictionary_t, const char *,
				 const char *);
static void	parse_int_param(prop_dictionary_t, const char *,
				const char *);
static void	parse_param(prop_dictionary_t, const char *,
			    void (*)(prop_dictionary_t, const char *,
			    const char *));
static void	parse_string_param(prop_dictionary_t, const char *,
				   const char *);
static void	usage(void) __dead;
static void	merge_dicts(prop_dictionary_t, const prop_dictionary_t);

int
main(int argc, char **argv)
{
	SIMPLEQ_HEAD(del_head, del_item) del_head;
	modctl_load_t cmdargs;
	prop_dictionary_t ext_props, props;
	bool del_props, merge_props, output_props;
	const char *ext_file;
	char *propsstr;
	int ch;
	int flags;

	struct del_item {
		SIMPLEQ_ENTRY(del_item) del_items;
		const char *del_key;
	} *delp;

	SIMPLEQ_INIT(&del_head);
	ext_file = NULL;
	ext_props = NULL;
	props = prop_dictionary_create();
	del_props = merge_props = output_props = false;
	flags = 0;

	while ((ch = getopt(argc, argv, "Pb:d:fi:m:ps:")) != -1) {
		switch (ch) {
		case 'P':
			flags |= MODCTL_NO_PROP;
			break;
		case 'b':
			parse_param(props, optarg, parse_bool_param);
			break;

		case 'd':
			del_props = true;
			delp = malloc(sizeof(struct del_item));
			if (delp == NULL)
				errx(EXIT_FAILURE, "Out of memory");
			delp->del_key = optarg;
			SIMPLEQ_INSERT_TAIL(&del_head, delp, del_items);
			break;

		case 'f':
			flags |= MODCTL_LOAD_FORCE;
			break;

		case 'i':
			parse_param(props, optarg, parse_int_param);
			break;

		case 'm':
			merge_props = true;
			ext_file = optarg;
			break;

		case 'p':
			output_props = true;
			break;

		case 's':
			parse_param(props, optarg, parse_string_param);
			break;

		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	propsstr = prop_dictionary_externalize(props);
	if (propsstr == NULL)
		errx(EXIT_FAILURE, "Failed to process properties");

	if (output_props) {
		if (merge_props) {
			ext_props =
			    prop_dictionary_internalize_from_file(ext_file);
			if (ext_props == NULL) {
				errx(EXIT_FAILURE, "Failed to read existing "
				    "property list");
			}

			free(propsstr);
			merge_dicts(ext_props, props);

			if (del_props)
				SIMPLEQ_FOREACH(delp, &del_head, del_items)
					prop_dictionary_remove(ext_props,
					    delp->del_key);

			propsstr = prop_dictionary_externalize(ext_props);
			if (propsstr == NULL)
				errx(EXIT_FAILURE, "Failed to process "
				    "properties");
		}
				
		fputs(propsstr, stdout);
	} else {
		if (argc != 1)
			usage();
		if (prog_init && prog_init() == -1)
			err(1, "prog init failed");
		cmdargs.ml_filename = argv[0];
		cmdargs.ml_flags = flags;
		cmdargs.ml_props = propsstr;
		cmdargs.ml_propslen = strlen(propsstr);

		if (prog_modctl(MODCTL_LOAD, &cmdargs)) {
			err(EXIT_FAILURE, NULL);
		}
	}

	free(propsstr);
	prop_object_release(props);

	exit(EXIT_SUCCESS);
}

static void
parse_bool_param(prop_dictionary_t props, const char *name,
		 const char *value)
{
	bool boolvalue;
	prop_object_t po;

	assert(name != NULL);
	assert(value != NULL);

	if (strcasecmp(value, "1") == 0 ||
	    strcasecmp(value, "true") == 0 ||
	    strcasecmp(value, "yes") == 0)
		boolvalue = true;
	else if (strcasecmp(value, "0") == 0 ||
	    strcasecmp(value, "false") == 0 ||
	    strcasecmp(value, "no") == 0)
		boolvalue = false;
	else
		errx(EXIT_FAILURE, "Invalid boolean value `%s'", value);

	po = prop_bool_create(boolvalue);
	if (po == NULL)
		err(EXIT_FAILURE, "prop_bool_create");
	prop_dictionary_set(props, name, po);
}

static void
parse_int_param(prop_dictionary_t props, const char *name,
		const char *value)
{
	int64_t intvalue;
	prop_object_t po;

	assert(name != NULL);
	assert(value != NULL);

	if (dehumanize_number(value, &intvalue) != 0)
		err(EXIT_FAILURE, "Invalid integer value `%s'", value);

	po = prop_number_create_integer(intvalue);
	if (po == NULL)
		err(EXIT_FAILURE, "prop_number_create_integer");
	prop_dictionary_set(props, name, po);
}

static void
parse_param(prop_dictionary_t props, const char *origstr,
	    void (*fmt_handler)(prop_dictionary_t, const char *, const char *))
{
	char *name, *value;

	name = strdup(origstr);

	value = strchr(name, '=');
	if (value == NULL) {
		free(name);
		errx(EXIT_FAILURE, "Invalid parameter `%s'", origstr);
	}
	*value++ = '\0';

	fmt_handler(props, name, value);

	free(name);
}

static void
parse_string_param(prop_dictionary_t props, const char *name,
		   const char *value)
{
	prop_object_t po;

	assert(name != NULL);
	assert(value != NULL);

	po = prop_string_create_cstring(value);
	if (po == NULL)
		err(EXIT_FAILURE, "prop_string_create_cstring");
	prop_dictionary_set(props, name, po);
}

static void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s [-fP] [-b var=boolean] [-i var=integer] "
	    "[-s var=string] module\n"
	    "       %s -p [-b var=boolean] [-d var] [-i var=integer] "
	    "[-m plist]\n               [-s var=string]\n",
	    getprogname(), getprogname());
	exit(EXIT_FAILURE);
}

static void
merge_dicts(prop_dictionary_t existing_dict, const prop_dictionary_t new_dict)
{
	prop_dictionary_keysym_t props_keysym;
	prop_object_iterator_t props_iter;
	prop_object_t props_obj;
	const char *props_key;

	props_iter = prop_dictionary_iterator(new_dict);
	if (props_iter == NULL) {
		errx(EXIT_FAILURE, "Failed to iterate new property list");
	}

	while ((props_obj = prop_object_iterator_next(props_iter)) != NULL) {
		props_keysym = (prop_dictionary_keysym_t)props_obj;
		props_key = prop_dictionary_keysym_cstring_nocopy(props_keysym);
		props_obj = prop_dictionary_get_keysym(new_dict, props_keysym);
		if ((props_obj == NULL) || !prop_dictionary_set(existing_dict,
		    props_key, props_obj)) {
			errx(EXIT_FAILURE, "Failed to copy "
			    "existing property list");
		}
	}
	prop_object_iterator_release(props_iter);

	return;
}
