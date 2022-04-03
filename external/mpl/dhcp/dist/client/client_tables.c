/*	$NetBSD: client_tables.c,v 1.3 2022/04/03 01:10:57 christos Exp $	*/

/* client_tables.c

   Tables of information only used by client... */

/*
 * Copyright (C) 2017-2022 Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   PO Box 360
 *   Newmarket, NH 03857 USA
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: client_tables.c,v 1.3 2022/04/03 01:10:57 christos Exp $");

#include "dhcpd.h"

struct universe client_universe;
static struct option client_options[] = {
    /* @todo dummy-client-parm should be removed with the first real param */
	{ "dummy-client-parm", "T",		&client_universe,   1, 1 },
	{ NULL, NULL, NULL, 0, 0 }
};

#define CLIENT_HASH_SIZE (2*(sizeof(client_options) / sizeof(struct option)))

void initialize_client_option_spaces(void);
void initialize_client_option_spaces(void)
{
	int i;

	/* Set up the client option universe... */
	client_universe.name = "client";
	client_universe.concat_duplicates = 0;
	client_universe.lookup_func = lookup_hashed_option;
	client_universe.option_state_dereference =
		hashed_option_state_dereference;
	client_universe.save_func = save_hashed_option;
	client_universe.delete_func = delete_hashed_option;
	client_universe.encapsulate = hashed_option_space_encapsulate;
	client_universe.foreach = hashed_option_space_foreach;
	client_universe.length_size = 1; /* Never used ... */
	client_universe.tag_size = 4;
	client_universe.store_tag = putUChar;
	client_universe.store_length = putUChar;
	client_universe.site_code_min = 0;
	client_universe.end = 0;
	client_universe.index = universe_count++;
	universes [client_universe.index] = &client_universe;
	if (!option_name_new_hash(&client_universe.name_hash,
				  CLIENT_HASH_SIZE, MDL) ||
	    !option_code_new_hash(&client_universe.code_hash,
				  CLIENT_HASH_SIZE, MDL))
		log_fatal ("Can't allocate client option hash table.");
	for (i = 0 ; client_options[i].name ; i++) {
		option_code_hash_add(client_universe.code_hash,
				     &client_options[i].code, 0,
				     &client_options[i], MDL);
		option_name_hash_add(client_universe.name_hash,
				     client_options[i].name, 0,
				     &client_options[i], MDL);
	}

	/* Add the client option space to the option space hash. */
	universe_hash_add (universe_hash,
			   client_universe.name, 0, &client_universe, MDL);

	/* Make the client universe the configuration option universe. */
	config_universe = &client_universe;
}
