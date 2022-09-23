/*	$NetBSD: sym_test.c,v 1.6 2022/09/23 12:15:23 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdbool.h>
#include <string.h>

#include <isc/commandline.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/symtab.h>
#include <isc/util.h>

isc_mem_t *mctx;
isc_symtab_t *st;

static void
undefine_action(char *key, unsigned int type, isc_symvalue_t value, void *arg) {
	UNUSED(arg);

	INSIST(type == 1);
	isc_mem_free(mctx, key);
	isc_mem_free(mctx, value.as_pointer);
}

int
main(int argc, char *argv[]) {
	char s[1000], *cp, *key;
	size_t len;
	isc_result_t result;
	isc_symvalue_t value;
	int trace = 0;
	int c;
	isc_symexists_t exists_policy = isc_symexists_reject;
	bool case_sensitive = false;

	while ((c = isc_commandline_parse(argc, argv, "tarc")) != -1) {
		switch (c) {
		case 't':
			trace = 1;
			break;
		case 'a':
			exists_policy = isc_symexists_add;
			break;
		case 'r':
			exists_policy = isc_symexists_replace;
			break;
		case 'c':
			case_sensitive = true;
			break;
		}
	}

	isc_mem_create(&mctx);
	RUNTIME_CHECK(isc_symtab_create(mctx, 691, undefine_action, NULL,
					case_sensitive, &st) == ISC_R_SUCCESS);

	while (fgets(s, sizeof(s), stdin) != NULL) {
		len = strlen(s);
		if (len > 0U && s[len - 1] == '\n') {
			s[len - 1] = '\0';
			len--;
		}

		cp = s;

		if (cp[0] == '!') {
			cp++;
			result = isc_symtab_undefine(st, cp, 1);
			if (trace || result != ISC_R_SUCCESS) {
				printf("undefine('%s'): %s\n", cp,
				       isc_result_totext(result));
			}
		} else {
			key = cp;
			while (*cp != '\0' && *cp != ' ' && *cp != '\t') {
				cp++;
			}
			if (*cp == '\0') {
				result = isc_symtab_lookup(st, key, 0, &value);
				if (trace || result != ISC_R_SUCCESS) {
					printf("lookup('%s'): %s", key,
					       isc_result_totext(result));
					if (result == ISC_R_SUCCESS) {
						cp = value.as_pointer;
						printf(", value == '%s'", cp);
					}
					printf("\n");
				}
			} else {
				*cp++ = '\0';
				key = isc_mem_strdup(mctx, key);
				value.as_pointer = isc_mem_strdup(mctx, cp);
				result = isc_symtab_define(st, key, 1, value,
							   exists_policy);
				if (trace || result != ISC_R_SUCCESS) {
					printf("define('%s', '%s'): %s\n", key,
					       cp, isc_result_totext(result));
					if (result != ISC_R_SUCCESS) {
						undefine_action(key, 1, value,
								NULL);
					}
				}
			}
		}
	}

	isc_symtab_destroy(&st);
	isc_mem_stats(mctx, stdout);
	isc_mem_destroy(&mctx);

	return (0);
}
