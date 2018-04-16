/*	$NetBSD: array.c,v 1.2.2.2 2018/04/16 01:59:48 pgoyette Exp $	*/

/* listener.c

   Subroutines that support the omapi extensible array type. */

/*
 * Copyright (c) 2004-2017 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2001-2003 by Internet Software Consortium
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
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: array.c,v 1.2.2.2 2018/04/16 01:59:48 pgoyette Exp $");

#include "dhcpd.h"

#include <omapip/omapip_p.h>

/* Allocate a new extensible array. */

isc_result_t omapi_array_allocate (omapi_array_t **array,
				   omapi_array_ref_t ref,
				   omapi_array_deref_t deref,
				   const char *file, int line)
{
	omapi_array_t *aptr;

	if (!array || *array)
		return DHCP_R_INVALIDARG;
	aptr = dmalloc (sizeof (omapi_array_t),file, line);
	if (!aptr)
		return ISC_R_NOMEMORY;
	*array = aptr;
	aptr -> ref = ref;
	aptr -> deref = deref;
	return ISC_R_SUCCESS;
}

isc_result_t omapi_array_free (omapi_array_t **array,
			       const char *file, int line)
{
	omapi_array_t *aptr;
	int i;

	if (!array || !*array)
		return DHCP_R_INVALIDARG;
	aptr = *array;
	for (i = 0; i < aptr -> count; i++)
		if (aptr -> data [i] && aptr -> deref)
			(*aptr -> deref) (&aptr -> data [i], file, line);
	dfree (aptr -> data, MDL);
	dfree (aptr, MDL);
	*array = (omapi_array_t *)0;
	return ISC_R_SUCCESS;
}

/* Extend the size of the array by one entry (we may allocate more than that)
   and store the specified value in the new array element. */

isc_result_t omapi_array_extend (omapi_array_t *array, char *ptr,
				 int *index, const char *file, int line)
{
	isc_result_t status;
	int new = array -> count;
	status = omapi_array_set (array, ptr, new, file, line);
	if (index && status == ISC_R_SUCCESS)
		*index = new;
	return status;
}

/* Set a value in the specified array, extending it if necessary. */

isc_result_t omapi_array_set (omapi_array_t *array, void *ptr, int index,
			      const char *file, int line)
{
	char **newbuf;
	int delta;
	isc_result_t status;

	if (!array)
		return DHCP_R_INVALIDARG;
	if (!ptr)
		return DHCP_R_INVALIDARG;
	if (index < 0)
		return DHCP_R_INVALIDARG;

	/* If the proposed index is larger than the current available
	   space in the array, make more space in the array. */
	if (array -> max <= index) {
		delta = index - array -> max + 10;
		newbuf = dmalloc ((array -> max + delta) * sizeof (char *),
				  file, line);
		if (!newbuf)
			return ISC_R_NOMEMORY;
		/* Zero the new elements. */
		memset (&newbuf [array -> max], 0, (sizeof (char *)) * delta);
		array -> max += delta;
		/* Copy the old array data into the new buffer. */
		if (array -> data) {
		    memcpy (newbuf,
			    array -> data, array -> count * sizeof (char *));
		    dfree (array -> data, file, line);
		}
		array -> data = newbuf;
	} else {
		/* If there's already data there, and this is an array
		   of references, dereference what's there. */
		if (array -> data [index]) {
			status = ((*array -> deref) (&array -> data [index],
						     file, line));
		
			if (status != ISC_R_SUCCESS)
				return status;
		}
	}

	/* Store the pointer using the referencer function.  We have
	   either just memset this to zero or dereferenced what was
	   there previously, so there is no need to do anything if the
	   pointer we have been asked to store is null. */
	if (ptr) {
		status = (*array -> ref) (&array -> data [index], ptr,
					  file, line);
		if (status != ISC_R_SUCCESS)
			return status;
	}
	if (index >= array -> count)
		array -> count = index + 1;
	return ISC_R_SUCCESS;
}

isc_result_t omapi_array_lookup (char **ptr, omapi_array_t *array, int index,
				 const char *file, int line)
{
	if (!array || !ptr || *ptr || index < 0 || index >= array -> count)
		return DHCP_R_INVALIDARG;
	if (array -> data [index])
		return (*array -> ref) (ptr,
					array -> data [index], file, line);
	return ISC_R_NOTFOUND;
}

