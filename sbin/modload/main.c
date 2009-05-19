/*	$NetBSD: main.c,v 1.7 2009/05/19 22:55:24 jnemeth Exp $	*/

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
__RCSID("$NetBSD: main.c,v 1.7 2009/05/19 22:55:24 jnemeth Exp $");
#endif /* !lint */

#include <sys/module.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include <prop/proplib.h>

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

int
main(int argc, char **argv)
{
	modctl_load_t cmdargs;
	prop_dictionary_t props;
	bool output_props = false;
	char *propsstr;
	int ch;
	int flags;

	flags = 0;
	props = prop_dictionary_create();

	while ((ch = getopt(argc, argv, "b:fi:ps:")) != -1) {
		switch (ch) {
		case 'b':
			parse_param(props, optarg, parse_bool_param);
			break;

		case 'f':
			flags |= MODCTL_LOAD_FORCE;
			break;

		case 'i':
			parse_param(props, optarg, parse_int_param);
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

	if (output_props)
		fputs(propsstr, stdout);
	else {
		if (argc != 1)
			usage();
		cmdargs.ml_filename = argv[0];
		cmdargs.ml_flags = flags;
		cmdargs.ml_props = propsstr;
		cmdargs.ml_propslen = strlen(propsstr);

		if (modctl(MODCTL_LOAD, &cmdargs)) {
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

	prop_dictionary_set(props, name, prop_bool_create(boolvalue));
}

static void
parse_int_param(prop_dictionary_t props, const char *name,
		const char *value)
{
	int64_t intvalue;

	assert(name != NULL);
	assert(value != NULL);

	if (dehumanize_number(value, &intvalue) != 0)
		err(EXIT_FAILURE, "Invalid integer value `%s'", value);

	prop_dictionary_set(props, name,
	    prop_number_create_integer(intvalue));
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

	assert(name != NULL);
	assert(value != NULL);

	prop_dictionary_set(props, name, prop_string_create_cstring(value));
}

static void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s [-f] [-b var=boolean] [-i var=integer] "
	    "[-s var=string] module\n"
	    "       %s -p [-b var=boolean] [-i var=integer] "
	    "[-s var=string]\n",
	    getprogname(), getprogname());
	exit(EXIT_FAILURE);
}
