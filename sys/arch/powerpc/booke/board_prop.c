/*	$NetBSD: board_prop.c,v 1.2 2011/01/18 01:02:52 matt Exp $	*/

/*
 * Copyright (c) 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: board_prop.c,v 1.2 2011/01/18 01:02:52 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <prop/proplib.h>

#include <powerpc/booke/cpuvar.h>

prop_dictionary_t board_properties;

void
board_info_init(void)
{

	/*
	 * Set up the board properties dictionary.
	 */
	if (board_properties != NULL)
		return;
	board_properties = prop_dictionary_create();
	KASSERT(board_properties != NULL);
}

bool
board_info_get_bool(const char *name)
{
	KASSERT(board_properties != NULL);
	prop_bool_t pb = prop_dictionary_get(board_properties, name);
	if (pb == NULL)
		return false;
	const bool value = prop_bool_true(pb);
	/* XXX -- do we need object release pb? */
	return value;
}

void
board_info_add_bool(const char *name)
{
	KASSERT(board_properties != NULL);
	prop_bool_t pb = prop_bool_create(true);
	KASSERT(pb != NULL);
	if (prop_dictionary_set(board_properties, name, pb) == false)
		panic("%s: setting %s", __func__, name);
	prop_object_release(pb);
}

uint64_t
board_info_get_number(const char *name)
{
	KASSERT(board_properties != NULL);
	prop_number_t pn = prop_dictionary_get(board_properties, name);
	KASSERT(pn != NULL);
	const uint64_t number = prop_number_unsigned_integer_value(pn);
	/* XXX -- do we need object release pn? */
	return number;
}

void
board_info_add_number(const char *name, uint64_t number)
{
	KASSERT(board_properties != NULL);
	prop_number_t pn = prop_number_create_integer(number);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, name, pn) == false)
		panic("%s: setting %s failed", __func__, name);
	prop_object_release(pn);
}

void
board_info_add_data(const char *name, const void *data, size_t len)
{
	KASSERT(board_properties != NULL);
	prop_data_t pd = prop_data_create_data(data, len);
	KASSERT(pd != NULL);
	if (prop_dictionary_set(board_properties, name, pd) == false)
		panic("%s: setting %s failed", __func__, name);
	prop_object_release(pd);
}

const void *
board_info_get_data(const char *name, size_t *lenp)
{
	KASSERT(board_properties != NULL);
	prop_data_t pd = prop_dictionary_get(board_properties, name);
	KASSERT(pd != NULL);
	*lenp = prop_data_size(pd);
	/* XXX -- do we need object release pn? */
	return prop_data_data(pd);
}

void
board_info_add_string(const char *name, const char *data)
{
	KASSERT(board_properties != NULL);
	prop_string_t ps = prop_string_create_cstring(data);
	KASSERT(ps != NULL);
	if (prop_dictionary_set(board_properties, name, ps) == false)
		panic("%s: setting %s failed", __func__, name);
	prop_object_release(ps);
}

void
board_info_add_object(const char *name, void *obj)
{
	if (prop_dictionary_set(board_properties, name, obj) == false)
		panic("%s: setting %s failed", __func__, name);
}

void *
board_info_get_object(const char *name)
{
	return prop_dictionary_get(board_properties, name);
}
