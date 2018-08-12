/*	$NetBSD: name_test.c,v 1.1.1.1 2018/08/12 12:07:39 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <config.h>

#include <stdlib.h>

#include <isc/commandline.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/result.h>

static void
print_wirename(isc_region_t *name) {
	unsigned char *ccurr, *cend;

	if (name->length == 0) {
		printf("<empty wire name>\n");
		return;
	}
	ccurr = name->base;
	cend = ccurr + name->length;
	while (ccurr != cend)
		printf("%02x ", *ccurr++);
	printf("\n");
}

static void
print_name(dns_name_t *name) {
	isc_result_t result;
	isc_buffer_t source;
	isc_region_t r;
	char s[1000];

	isc_buffer_init(&source, s, sizeof(s));
	if (dns_name_countlabels(name) > 0)
		result = dns_name_totext(name, ISC_FALSE, &source);
	else
		result = ISC_R_SUCCESS;
	if (result == ISC_R_SUCCESS) {
		isc_buffer_usedregion(&source, &r);
		if (r.length > 0)
			printf("%.*s\n", (int)r.length, r.base);
		else
			printf("<empty text name>\n");
	} else
		printf("error: %s\n", dns_result_totext(result));
}

int
main(int argc, char *argv[]) {
	char s[1000];
	isc_result_t result;
	dns_fixedname_t wname, wname2, oname, compname, downname;
	isc_buffer_t source;
	isc_region_t r;
	dns_name_t *name, *comp, *down;
	const dns_name_t *origin;
	unsigned int downcase = 0;
	size_t len;
	isc_boolean_t quiet = ISC_FALSE;
	isc_boolean_t concatenate = ISC_FALSE;
	isc_boolean_t got_name = ISC_FALSE;
	isc_boolean_t check_absolute = ISC_FALSE;
	isc_boolean_t check_wildcard = ISC_FALSE;
	isc_boolean_t test_downcase = ISC_FALSE;
	isc_boolean_t inplace = ISC_FALSE;
	isc_boolean_t want_split = ISC_FALSE;
	unsigned int labels, split_label = 0;
	dns_fixedname_t fprefix, fsuffix;
	dns_name_t *prefix, *suffix;
	int ch;

	while ((ch = isc_commandline_parse(argc, argv, "acdiqs:w")) != -1) {
		switch (ch) {
		case 'a':
			check_absolute = ISC_TRUE;
			break;
		case 'c':
			concatenate = ISC_TRUE;
			break;
		case 'd':
			test_downcase = ISC_TRUE;
			break;
		case 'i':
			inplace = ISC_TRUE;
			break;
		case 'q':
			quiet = ISC_TRUE;
			break;
		case 's':
			want_split = ISC_TRUE;
			split_label = atoi(isc_commandline_argument);
			break;
		case 'w':
			check_wildcard = ISC_TRUE;
			break;
		}
	}

	argc -= isc_commandline_index;
	argv += isc_commandline_index;

	if (argc > 0) {
		if (strcasecmp("none", argv[0]) == 0)
			origin = NULL;
		else {
			len = strlen(argv[0]);
			isc_buffer_init(&source, argv[0], len);
			isc_buffer_add(&source, len);
			dns_fixedname_init(&oname);
			result = dns_name_fromtext(dns_fixedname_name(&oname),
						   &source, dns_rootname, 0,
						   NULL);
			if (result != 0) {
				fprintf(stderr,
					"dns_name_fromtext() failed: %s\n",
					dns_result_totext(result));
				exit(1);
			}
			origin = dns_fixedname_name(&oname);
		}
	} else if (concatenate)
		origin = NULL;
	else
		origin = dns_rootname;

	if (argc >= 1) {
		if (strcasecmp("none", argv[1]) == 0)
			comp = NULL;
		else {
			len = strlen(argv[1]);
			isc_buffer_init(&source, argv[1], len);
			isc_buffer_add(&source, len);
			comp = dns_fixedname_initname(&compname);
			result = dns_name_fromtext(comp, &source, origin,
						   0, NULL);
			if (result != 0) {
				fprintf(stderr,
					"dns_name_fromtext() failed: %s\n",
					dns_result_totext(result));
				exit(1);
			}
		}
	} else
		comp = NULL;

	name = dns_fixedname_initname(&wname);
	dns_fixedname_init(&wname2);
	while (fgets(s, sizeof(s), stdin) != NULL) {
		len = strlen(s);
		if (len > 0U && s[len - 1] == '\n') {
			s[len - 1] = '\0';
			len--;
		}
		isc_buffer_init(&source, s, len);
		isc_buffer_add(&source, len);

		if (len > 0U)
			result = dns_name_fromtext(name, &source, origin,
						   downcase, NULL);
		else {
			if (name == dns_fixedname_name(&wname))
				dns_fixedname_init(&wname);
			else
				dns_fixedname_init(&wname2);
			result = ISC_R_SUCCESS;
		}

		if (result != ISC_R_SUCCESS) {
			printf("%s\n", dns_result_totext(result));
			if (name == dns_fixedname_name(&wname))
				dns_fixedname_init(&wname);
			else
				dns_fixedname_init(&wname2);
			continue;
		}

		if (check_absolute && dns_name_countlabels(name) > 0) {
			if (dns_name_isabsolute(name))
				printf("absolute\n");
			else
				printf("relative\n");
		}
		if (check_wildcard && dns_name_countlabels(name) > 0) {
			if (dns_name_iswildcard(name))
				printf("wildcard\n");
			else
				printf("not wildcard\n");
		}
		dns_name_toregion(name, &r);
		if (!quiet) {
			print_wirename(&r);
			printf("%u labels, %u bytes.\n",
			       dns_name_countlabels(name), r.length);
		}

		if (concatenate) {
			if (got_name) {
				printf("Concatenating.\n");
				result = dns_name_concatenate(
						   dns_fixedname_name(&wname),
						   dns_fixedname_name(&wname2),
						   dns_fixedname_name(&wname2),
						   NULL);
				name = dns_fixedname_name(&wname2);
				if (result == ISC_R_SUCCESS) {
					if (check_absolute &&
					    dns_name_countlabels(name) > 0) {
						if (dns_name_isabsolute(name))
							printf("absolute\n");
						else
							printf("relative\n");
					}
					if (check_wildcard &&
					    dns_name_countlabels(name) > 0) {
						if (dns_name_iswildcard(name))
							printf("wildcard\n");
						else
							printf("not "
							       "wildcard\n");
					}
					dns_name_toregion(name, &r);
					if (!quiet) {
						print_wirename(&r);
						printf("%u labels, "
						       "%u bytes.\n",
						   dns_name_countlabels(name),
						       r.length);
					}
				} else
					printf("%s\n",
					       dns_result_totext(result));
				got_name = ISC_FALSE;
			} else
				got_name = ISC_TRUE;
		}
		isc_buffer_init(&source, s, sizeof(s));
		if (dns_name_countlabels(name) > 0)
			result = dns_name_totext(name, ISC_FALSE, &source);
		else
			result = ISC_R_SUCCESS;
		if (result == ISC_R_SUCCESS) {
			isc_buffer_usedregion(&source, &r);
			if (r.length > 0)
				printf("%.*s\n", (int)r.length, r.base);
			else
				printf("<empty text name>\n");
			if (!quiet) {
				printf("%u bytes.\n", source.used);
			}
		} else
			printf("%s\n", dns_result_totext(result));

		if (test_downcase) {
			if (inplace) {
				down = name;
			} else {
				down = dns_fixedname_initname(&downname);
			}
			result = dns_name_downcase(name, down, NULL);
			INSIST(result == ISC_R_SUCCESS);
			if (!quiet) {
				dns_name_toregion(down, &r);
				print_wirename(&r);
				printf("%u labels, %u bytes.\n",
				       dns_name_countlabels(down),
				       r.length);
			}
			isc_buffer_init(&source, s, sizeof(s));
			print_name(down);
		}

		if (comp != NULL && dns_name_countlabels(name) > 0) {
			int order;
			unsigned int nlabels;
			dns_namereln_t namereln;

			namereln = dns_name_fullcompare(name, comp, &order,
							&nlabels);
			if (!quiet) {
				if (order < 0)
					printf("<");
				else if (order > 0)
					printf(">");
				else
					printf("=");
				switch (namereln) {
				case dns_namereln_contains:
					printf(", contains");
					break;
				case dns_namereln_subdomain:
					printf(", subdomain");
					break;
				case dns_namereln_commonancestor:
					printf(", common ancestor");
					break;
				default:
					break;
				}
				if (namereln != dns_namereln_none &&
				    namereln != dns_namereln_equal)
					printf(", nlabels = %u", nlabels);
				printf("\n");
			}
			printf("dns_name_equal() returns %s\n",
			       dns_name_equal(name, comp) ? "TRUE" : "FALSE");
		}

		labels = dns_name_countlabels(name);
		if (want_split && split_label < labels) {
			prefix = dns_fixedname_initname(&fprefix);
			suffix = dns_fixedname_initname(&fsuffix);
			printf("splitting at label %u: ", split_label);
			dns_name_split(name, split_label, prefix, suffix);
			printf("\n    prefix = ");
			print_name(prefix);
			printf("    suffix = ");
			print_name(suffix);
		}

		if (concatenate) {
			if (got_name)
				name = dns_fixedname_name(&wname2);
			else
				name = dns_fixedname_name(&wname);
		}
	}

	return (0);
}
