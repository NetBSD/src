/* $NetBSD: drvctl.c,v 1.20 2018/02/14 17:43:09 jakllsch Exp $ */

/*
 * Copyright (c) 2004
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/drvctlio.h>


#define	OPEN_MODE(mode)							\
	(((mode) == 'd' || (mode) == 'r') ? O_RDWR			\
					  : O_RDONLY)

__dead static void usage(void);
static void extract_property(prop_dictionary_t, const char *, bool);
static void display_object(prop_object_t, bool);
static void list_children(int, char *, bool, bool, int);

static void
usage(void)
{
	const char *p = getprogname();

	fprintf(stderr, "Usage: %s -r [-a attribute] busdevice [locator ...]\n"
	    "       %s -d device\n"
	    "       %s [-nt] -l [device]\n"
	    "       %s [-n] -p device [property]\n"
	    "       %s -Q device\n"
	    "       %s -R device\n"
	    "       %s -S device\n", p, p, p, p, p, p, p);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	bool nflag = false, tflag = false;
	int c, mode;
	char *attr = 0;
	extern char *optarg;
	extern int optind;
	int fd, res;
	struct devpmargs paa = { .devname = "", .flags = 0 };
	struct devdetachargs daa;
	struct devrescanargs raa;
	int *locs, i;
	prop_dictionary_t command_dict, args_dict, results_dict, data_dict;
	prop_string_t string;
	prop_number_t number;
	char *xml;

	mode = 0;
	while ((c = getopt(argc, argv, "QRSa:dlnprt")) != -1) {
		switch (c) {
		case 'Q':
		case 'R':
		case 'S':
		case 'd':
		case 'l':
		case 'p':
		case 'r':
			mode = c;
			break;
		case 'a':
			attr = optarg;
			break;
		case 'n':
			nflag = true;
			break;
		case 't':
			tflag = nflag = true;
			break;
		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if ((argc < 1 && mode != 'l') || mode == 0)
		usage();

	fd = open(DRVCTLDEV, OPEN_MODE(mode), 0);
	if (fd == -1)
		err(EXIT_FAILURE, "open %s", DRVCTLDEV);

	switch (mode) {
	case 'Q':
		paa.flags = DEVPM_F_SUBTREE;
		/*FALLTHROUGH*/
	case 'R':
		strlcpy(paa.devname, argv[0], sizeof(paa.devname));

		if (ioctl(fd, DRVRESUMEDEV, &paa) == -1)
			err(EXIT_FAILURE, "DRVRESUMEDEV");
		break;
	case 'S':
		strlcpy(paa.devname, argv[0], sizeof(paa.devname));

		if (ioctl(fd, DRVSUSPENDDEV, &paa) == -1)
			err(EXIT_FAILURE, "DRVSUSPENDDEV");
		break;
	case 'd':
		strlcpy(daa.devname, argv[0], sizeof(daa.devname));

		if (ioctl(fd, DRVDETACHDEV, &daa) == -1)
			err(EXIT_FAILURE, "DRVDETACHDEV");
		break;
	case 'l':
		list_children(fd, argc ? argv[0] : NULL, nflag, tflag, 0);
		break;
	case 'r':
		memset(&raa, 0, sizeof(raa));
		strlcpy(raa.busname, argv[0], sizeof(raa.busname));
		if (attr)
			strlcpy(raa.ifattr, attr, sizeof(raa.ifattr));
		if (argc > 1) {
			locs = malloc((argc - 1) * sizeof(int));
			if (!locs)
				err(EXIT_FAILURE, "malloc int[%d]", argc - 1);
			for (i = 0; i < argc - 1; i++)
				locs[i] = atoi(argv[i + 1]);
			raa.numlocators = argc - 1;
			raa.locators = locs;
		}

		if (ioctl(fd, DRVRESCANBUS, &raa) == -1)
			err(EXIT_FAILURE, "DRVRESCANBUS");
		break;
	case 'p':
		command_dict = prop_dictionary_create();
		args_dict = prop_dictionary_create();

		string = prop_string_create_cstring_nocopy("get-properties");
		prop_dictionary_set(command_dict, "drvctl-command", string);
		prop_object_release(string);

		string = prop_string_create_cstring(argv[0]);
		prop_dictionary_set(args_dict, "device-name", string);
		prop_object_release(string);

		prop_dictionary_set(command_dict, "drvctl-arguments",
		    args_dict);
		prop_object_release(args_dict);

		res = prop_dictionary_sendrecv_ioctl(command_dict, fd,
		    DRVCTLCOMMAND, &results_dict);
		prop_object_release(command_dict);
		if (res)
			errc(EXIT_FAILURE, res, "DRVCTLCOMMAND");

		number = prop_dictionary_get(results_dict, "drvctl-error");
		if (prop_number_integer_value(number) != 0) {
			errc(EXIT_FAILURE,
			    (int)prop_number_integer_value(number),
			    "get-properties");
		}

		data_dict = prop_dictionary_get(results_dict,
		    "drvctl-result-data");
		if (data_dict == NULL) {
			errx(EXIT_FAILURE,
			    "get-properties: failed to return result data");
		}

		if (argc == 1) {
			xml = prop_dictionary_externalize(data_dict);
			if (!nflag) {
				printf("Properties for device `%s':\n",
				    argv[0]);
			}
			printf("%s", xml);
			free(xml);
		} else {
			for (i = 1; i < argc; i++)
				extract_property(data_dict, argv[i], nflag);
		}

		prop_object_release(results_dict);
		break;
	default:
		errx(EXIT_FAILURE, "unknown command `%c'", mode);
	}

	return EXIT_SUCCESS;
}

static void
extract_property(prop_dictionary_t dict, const char *prop, bool nflag)
{
	char *s, *p, *cur, *ep = NULL;
	prop_object_t obj;
	unsigned long ind;

	obj = dict;
	cur = NULL;
	s = strdup(prop);
	p = strtok_r(s, "/", &ep);
	while (p) {
		cur = p;
		p = strtok_r(NULL, "/", &ep);

		switch (prop_object_type(obj)) {
		case PROP_TYPE_DICTIONARY:
			obj = prop_dictionary_get(obj, cur);
			if (obj == NULL)
				exit(EXIT_FAILURE);
			break;
		case PROP_TYPE_ARRAY:
			ind = strtoul(cur, NULL, 0);
			obj = prop_array_get(obj, ind);
			if (obj == NULL)
				exit(EXIT_FAILURE);
			break;
		default:
			errx(EXIT_FAILURE, "Select neither dict nor array with"
			" `%s'", cur);
		}
	}

	if (obj != NULL && cur != NULL)
		display_object(obj, nflag);

	free(s);
}

static void
display_object(prop_object_t obj, bool nflag)
{
	char *xml;
	prop_object_t next_obj;
	prop_object_iterator_t iter;

	if (obj == NULL)
		exit(EXIT_FAILURE);
	switch (prop_object_type(obj)) {
	case PROP_TYPE_BOOL:
		printf("%s\n", prop_bool_true(obj) ? "true" : "false");
		break;
	case PROP_TYPE_NUMBER:
		printf("%" PRId64 "\n", prop_number_integer_value(obj));
		break;
	case PROP_TYPE_STRING:
		printf("%s\n", prop_string_cstring_nocopy(obj));
		break;
	case PROP_TYPE_DICTIONARY:
		xml = prop_dictionary_externalize(obj);
		printf("%s", xml);
		free(xml);
		break;
	case PROP_TYPE_ARRAY:
		iter = prop_array_iterator(obj);
		if (!nflag)
			printf("Array:\n");
		while ((next_obj = prop_object_iterator_next(iter)) != NULL)
			display_object(next_obj, nflag);
		break;
	default:
		errx(EXIT_FAILURE, "Unhandled type %d", prop_object_type(obj));
	}
}

static void
list_children(int fd, char *dvname, bool nflag, bool tflag, int depth)
{
	struct devlistargs laa = {
	    .l_devname = "", .l_childname = NULL, .l_children = 0
	};
	size_t children;
	int i, n;

	if (dvname == NULL) {
		if (depth > 0)
			return;
		*laa.l_devname = '\0';
	} else {
		strlcpy(laa.l_devname, dvname, sizeof(laa.l_devname));
	}

	if (ioctl(fd, DRVLISTDEV, &laa) == -1)
		err(EXIT_FAILURE, "DRVLISTDEV");

	children = laa.l_children;

	laa.l_childname = malloc(children * sizeof(laa.l_childname[0]));
	if (laa.l_childname == NULL)
		err(EXIT_FAILURE, "DRVLISTDEV");
	if (ioctl(fd, DRVLISTDEV, &laa) == -1)
		err(EXIT_FAILURE, "DRVLISTDEV");
	if (laa.l_children > children)
		err(EXIT_FAILURE, "DRVLISTDEV: number of children grew");

	for (i = 0; i < (int)laa.l_children; i++) {
		for (n = 0; n < depth; n++)
			printf("  ");
		if (!nflag) {
			printf("%s ",
			    (dvname == NULL) ? "root" : laa.l_devname);
		}
		printf("%s\n", laa.l_childname[i]);
		if (tflag) {
			list_children(fd, laa.l_childname[i], nflag,
			    tflag, depth + 1);
		}
	}

	free(laa.l_childname);
}
