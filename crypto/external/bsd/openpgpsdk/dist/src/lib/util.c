/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 */

#include <openpgpsdk/util.h>
#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/create.h>
#include <openpgpsdk/errors.h>
#include <openpgpsdk/readerwriter.h>
#include <stdio.h>
#include <assert.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <string.h>

#include <openpgpsdk/final.h>

/**
 * Searches the given map for the given type.
 * Returns a human-readable descriptive string if found,
 * returns NULL if not found
 *
 * It is the responsibility of the calling function to handle the
 * error case sensibly (i.e. don't just print out the return string.
 *
 */
static const char *
str_from_map_or_null(int type, ops_map_t * map)
{
	ops_map_t      *row;

	for (row = map; row->string != NULL; row++)
		if (row->type == type)
			return row->string;
	return NULL;
}

/**
 * \ingroup Core_Print
 *
 * Searches the given map for the given type.
 * Returns a readable string if found, "Unknown" if not.
 */

const char     *
ops_str_from_map(int type, ops_map_t * map)
{
	const char     *str;
	str = str_from_map_or_null(type, map);
	if (str)
		return (str);
	else
		return ("Unknown");
}

void 
hexdump(const unsigned char *src, size_t length, const char *sep)
{
	unsigned i;

	for (i = 0 ; i < length ; i += 2) {
		printf("%02x", *src++);
		printf("%02x%s", *src++, sep);
	}
}

/**
 * \ingroup HighLevel_Functions
 * \brief Initialises OpenPGP::SDK. To be called before any other OPS function.
 *
 * Initialises OpenPGP::SDK and the underlying openssl library.
 */

void 
ops_init(void)
{
	ops_crypto_init();
}

/**
 * \ingroup HighLevel_Functions
 * \brief Closes down OpenPGP::SDK.
 *
 * Close down OpenPGP:SDK, release any resources under the control of
 * the library. No OpenPGP:SDK function other than ops_init() should
 * be called after this function.
 */

void 
ops_finish(void)
{
	ops_crypto_finish();
}

typedef struct {
	const unsigned char *buffer;
	size_t          length;
	size_t          offset;
}               reader_mem_arg_t;


typedef struct {
	unsigned short  sum;
}               sum16_arg_t;

static int 
sum16_reader(void *dest_, size_t length, ops_error_t ** errors,
	     ops_reader_info_t * rinfo, ops_parse_cb_info_t * cbinfo)
{
	const unsigned char *dest = dest_;
	sum16_arg_t    *arg = ops_reader_get_arg(rinfo);
	int             r = ops_stacked_read(dest_, length, errors, rinfo, cbinfo);
	int             n;

	if (r < 0)
		return r;

	for (n = 0; n < r; ++n)
		arg->sum = (arg->sum + dest[n]) & 0xffff;

	return r;
}

static void 
sum16_destroyer(ops_reader_info_t * rinfo)
{
	free(ops_reader_get_arg(rinfo));
}

/**
   \ingroup Internal_Readers_Sum16
   \param pinfo Parse settings
*/

void 
ops_reader_push_sum16(ops_parse_info_t * pinfo)
{
	sum16_arg_t    *arg = calloc(1, sizeof *arg);

	ops_reader_push(pinfo, sum16_reader, sum16_destroyer, arg);
}

/**
   \ingroup Internal_Readers_Sum16
   \param pinfo Parse settings
   \return sum
*/
unsigned short 
ops_reader_pop_sum16(ops_parse_info_t * pinfo)
{
	sum16_arg_t    *arg = ops_reader_get_arg(ops_parse_get_rinfo(pinfo));
	unsigned short  sum = arg->sum;

	ops_reader_pop(pinfo);
	free(arg);

	return sum;
}

/* small useful functions for setting the file-level debugging levels */
/* saves editing the static variable for every source file */
/* if the debugv list contains the filename in question, we're debugging it */

enum {
	MAX_DEBUG_NAMES = 32
};

static int      debugc;
static char    *debugv[MAX_DEBUG_NAMES];

/* set the debugging level per filename */
int
ops_set_debug_level(const char *f)
{
	const char     *name;
	int             i;

	if (f == NULL) {
		f = "all";
	}
	if ((name = strrchr(f, '/')) == NULL) {
		name = f;
	} else {
		name += 1;
	}
	for (i = 0; i < debugc && i < MAX_DEBUG_NAMES; i++) {
		if (strcmp(debugv[i], name) == 0) {
			return 1;
		}
	}
	if (i == MAX_DEBUG_NAMES) {
		return 0;
	}
	debugv[debugc++] = strdup(name);
	return 1;
}

/* get the debugging level per filename */
int
ops_get_debug_level(const char *f)
{
	const char     *name;
	int             i;

	if ((name = strrchr(f, '/')) == NULL) {
		name = f;
	} else {
		name += 1;
	}
	for (i = 0; i < debugc; i++) {
		if (strcmp(debugv[i], "all") == 0 ||
		    strcmp(debugv[i], name) == 0) {
			return 1;
		}
	}
	return 0;
}
